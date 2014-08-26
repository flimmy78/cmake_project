// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File        : udp_console_u.c
// Description : Linux console based on UDP protocol
// License     :
//
//  Copyright (C) 2011 Rachid Koucha <rachid dot koucha at free dot fr>
//
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Notes: This source code gets its roots from
//
//  http://kernelnewbies.org/Simple_UDP_Server for the networking services.
//
//
// Evolutions  :
//
//     19-Jan-2011  R. Koucha    - Creation
//
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/pid.h>
#include <linux/console.h>
#include <linux/completion.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/smp_lock.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>




// ----------------------------------------------------------------------------
// Name        : UDPC_ERR
// Description : Display of a message
// ----------------------------------------------------------------------------
#define UDPC_PRINT(level, format, ...)					\
           do {                                                           \
	     printk(level "UDPC(%s#%d): " format, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
              } while (0)



// ----------------------------------------------------------------------------
// Name        : UDPC_DEFAULT_CONNECT_PORT
// Description : Default UDP port to send the messages
// ----------------------------------------------------------------------------
#define UDPC_DEFAULT_CONNECT_PORT 23


// ----------------------------------------------------------------------------
// Name        : UDPC_DEFAULT_INADDR_SEND
// Description : Default IP address to send the messages
// ----------------------------------------------------------------------------
#define UDPC_DEFAULT_INADDR_SEND INADDR_LOOPBACK


// ----------------------------------------------------------------------------
// Name        : udpc_addr/port
// Description : Destination address/port
// ----------------------------------------------------------------------------
static __be32  udpc_addr = UDPC_DEFAULT_INADDR_SEND;
static __be16  udpc_port = UDPC_DEFAULT_CONNECT_PORT;


// ----------------------------------------------------------------------------
// Name        : UDPC_MODULE_NAME
// Description : Default module name
// ----------------------------------------------------------------------------
#define UDPC_MODULE_NAME "udpc"



// ----------------------------------------------------------------------------
// Name        : udpc_thd_info
// Description : Information for the thread
// ----------------------------------------------------------------------------
struct udpc_thd_info
{
  struct task_struct *thread_id;
  struct socket      *sock_send;
  struct sockaddr_in  addr_send;
  struct pid         *pid;
  struct completion   terminated;
};

static struct udpc_thd_info udpc_thd;



// ----------------------------------------------------------------------------
// Name        : udpc_proc_cnf
// Description : Configuration entry point in /proc
// ----------------------------------------------------------------------------
static struct proc_dir_entry *udpc_proc;


// ----------------------------------------------------------------------------
// Name        : udpc_log_wr/rd
// Description : Write/Read pointers in the LOG buffer
// ----------------------------------------------------------------------------
static unsigned int udpc_log_wr, udpc_log_rd;


// ----------------------------------------------------------------------------
// Name        : udpc_log_slot
// Description : Record in the LOG buffer
// ----------------------------------------------------------------------------
typedef struct udpc_log_slot
{
  size_t               len;
} udpc_log_slot_t;


// ----------------------------------------------------------------------------
// Name        : udpc_log_slot
// Description : The LOG buffer.
// Notes       : . printk()/vprintk() generate messages of a size no longer than
//                 1024 bytes (terminating NUL included !)
//               . The console subsystem add to it a weigth "<x>" and
//                 eventually a timestamp "[XXXXX.XXXXXX]" plus a space char
//               ==> The maximum size of a buffer is 1024 + 3 + 14 + 1 = 1042
// ----------------------------------------------------------------------------
#define UDPC_LOG_BUF_SHIFT    5    // 2^5 = 32 KB
#define UDPC_LOG_BUF_SZ       (1 << UDPC_LOG_BUF_SHIFT)
#define UDPC_LOG_BUF_SLOT_SZ  1042
static udpc_log_slot_t udpc_log_buf[UDPC_LOG_BUF_SZ];
static char            udpc_log_slot[UDPC_LOG_BUF_SZ][UDPC_LOG_BUF_SLOT_SZ];

#define UDPC_LOG_BUF_MASK   (UDPC_LOG_BUF_SZ - 1)    
#define UDPC_LOG_IDX(i)  (i & UDPC_LOG_BUF_MASK)

// ----------------------------------------------------------------------------
// Name        : udpc_log_sync
// Description : Synchronization with the console manager
// ----------------------------------------------------------------------------
static DECLARE_WAIT_QUEUE_HEAD(udpc_log_sync);
static int udpc_display = 0;




// ----------------------------------------------------------------------------
// Name        : udpc_log_write
// Description : Write a message into the LOG buffer
// ----------------------------------------------------------------------------
static void udpc_log_write(
			   const char   *msg,
			   size_t        len
                          )
{
char *p;

  BUG_ON(len > UDPC_LOG_BUF_SLOT_SZ);

  udpc_log_buf[UDPC_LOG_IDX(udpc_log_wr)].len = len;
  p = &(udpc_log_slot[UDPC_LOG_IDX(udpc_log_wr)][0]);
  memcpy(p, msg, len);

  udpc_log_wr ++;
  if ((udpc_log_wr - udpc_log_rd) > UDPC_LOG_BUF_SZ)
  {
    // Overwirte the oldest message
    udpc_log_rd = udpc_log_wr -  UDPC_LOG_BUF_SZ;
  }
} // udpc_log_write


// ----------------------------------------------------------------------------
// Name        : udpc_log_read
// Description : Read a message from the LOG buffer
// ----------------------------------------------------------------------------
static char *udpc_log_read(
			    size_t *len
                          )
{
char *p;

  if (udpc_log_wr - udpc_log_rd)
  {
    *len = udpc_log_buf[UDPC_LOG_IDX(udpc_log_rd)].len ;
    p = &(udpc_log_slot[UDPC_LOG_IDX(udpc_log_rd)][0]);

    udpc_log_rd ++;

    return p;
  }

  return NULL;
} // udpc_log_read


// ----------------------------------------------------------------------------
// Name        : udpc_console_write
// Description : Callback for the console subsystem to submit a message
// ----------------------------------------------------------------------------
static void udpc_console_write(
                               struct console *console,
                               const char     *s,
                               unsigned int    count
                                )
{
  (void)console;

  udpc_log_write(s, count);

  udpc_display = 1;
  wake_up_interruptible(&udpc_log_sync);
} // udpc_console_write


// ----------------------------------------------------------------------------
// Name   : udpc_console
// Usage  : Console descriptor
// ----------------------------------------------------------------------------
static struct console udpc_console =
{
  .name	 = "udpc",
  .write = udpc_console_write,
#if 0
  // CON_PRINTBUFFER triggers the print of all the buffered messages which
  // ends in displaying twice the startup messages on local console
  .flags = CON_PRINTBUFFER | CON_ENABLED,
#else
  .flags = CON_ENABLED,
#endif
  .index = -1,
};




// ----------------------------------------------------------------------------
// Name   : udpc_send
// Usage  : Send an UDP message
// Return : Number of sent bytes
// ----------------------------------------------------------------------------
static int udpc_send(
                     struct socket      *sock,
                     struct sockaddr_in *addr,
                     char               *buf,
                     int                 len
                    )
{
struct msghdr msg;
struct iovec  iov;
mm_segment_t  oldfs;
int           size = 0;

  if (NULL == sock->sk)
  {
    return 0;
  }

  iov.iov_base = buf;
  iov.iov_len  = len;

  msg.msg_flags      = 0;
  msg.msg_name       = addr;
  msg.msg_namelen    = sizeof(struct sockaddr_in);
  msg.msg_control    = NULL;
  msg.msg_controllen = 0;
  msg.msg_iov        = &iov;
  msg.msg_iovlen     = 1;
  msg.msg_control    = NULL;

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  size = sock_sendmsg(sock, &msg,len);
  set_fs(oldfs);

  return size;
} /* udpc_send */


// ----------------------------------------------------------------------------
// Name   : udpc_thd_entry
// Usage  : Thread's entry point
// ----------------------------------------------------------------------------
static void udpc_thd_entry(void *data)
{
int           rc;
size_t        len;
char         *p;
__be32        addr = udpc_addr;
__be16        port = udpc_port;

  (void)data;

  lock_kernel();

  /* kernel thread initialization */
  udpc_thd.pid = get_pid(task_pid(current));
  current->flags |= PF_NOFREEZE;

  /* daemonize (take care with signals, after daemonize() they are disabled) */
  daemonize(UDPC_MODULE_NAME);
  allow_signal(SIGKILL);

  unlock_kernel();

  /* Create the socket */
  rc = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &(udpc_thd.sock_send));
  if (rc < 0)
  {
    UDPC_PRINT(KERN_ERR, "Could not create a datagram socket, error = %d\n", -rc);
    goto out;
  }

  memset(&(udpc_thd.addr_send), 0, sizeof(struct sockaddr_in));
  udpc_thd.addr_send.sin_family      = AF_INET;
  udpc_thd.addr_send.sin_addr.s_addr = htonl(addr);
  udpc_thd.addr_send.sin_port        = htons(port);

  /* Main loop */
  while (1)
  {
    /* Wait for a message to display from the console subsystem */
    wait_event_interruptible(udpc_log_sync, udpc_display != 0);
    udpc_display = 0;

    if (signal_pending(current))
    {
      UDPC_PRINT(KERN_DEBUG, "Received a signal\n");
      break;
    }

    /* Check if destination port and/or address have changed */
    if (addr != udpc_addr || port != udpc_port)
    {
      addr = udpc_addr;
      port = udpc_port;
      memset(&(udpc_thd.addr_send), 0, sizeof(struct sockaddr_in));
      udpc_thd.addr_send.sin_family      = AF_INET;
      udpc_thd.addr_send.sin_addr.s_addr = htonl(addr);
      udpc_thd.addr_send.sin_port        = htons(port);
    }

    do
    {
      p = udpc_log_read(&len);
      if (p)
      {
        udpc_send(udpc_thd.sock_send, &(udpc_thd.addr_send), p, len);
      }
    } while(p);
  } /* End while */

  UDPC_PRINT(KERN_DEBUG, "Thread terminating\n");

  sock_release(udpc_thd.sock_send);
  udpc_thd.sock_send = NULL;

out:
  udpc_thd.thread_id = NULL;
  complete_and_exit(&(udpc_thd.terminated), 0);
} /* udpc_thd_entry */


// ----------------------------------------------------------------------------
// Name        : udpc_proc_read
// Description : Read entry point in /proc
// Return      : Number of read bytes, if OK
//               -errno, if error
// ----------------------------------------------------------------------------
static ssize_t udpc_proc_read(
                              struct file *file,
                              char __user *buf,
			      size_t       count,
                              loff_t      *ppos
                             )
{
ssize_t rc;
char    cnf[3 + 1 + 3 + 1 + 3 + 1 + 3 + 1 + 10 + 1 + 1];

  if (*ppos > 0)
  {
     return 0;
  }

  if (count < sizeof(cnf))
  {
    return -EINVAL;
  }

  rc = snprintf(cnf, sizeof(cnf), "%u.%u.%u.%u:%u\n"
                                ,
	                        udpc_addr >> 24 & 0xFF, udpc_addr >> 16 & 0xFF, udpc_addr >> 8 & 0xFF, udpc_addr & 0xFF,
                                udpc_port
               );

  if (copy_to_user(buf, cnf, rc))
  {
    return -EFAULT;
  }

  *ppos += rc;

  return rc;
} /* udpc_proc_read */



// ----------------------------------------------------------------------------
// Name        : udpc_proc_write
// Description : Read entry point in /proc
// Return      : Number of written bytes, if OK
//               -errno, if error
// ----------------------------------------------------------------------------
static ssize_t udpc_proc_write(
                               struct file       *file,
                               const char __user *buf,
                               size_t             count,
                               loff_t            *ppos
                              )
{
char         cnf[3 + 1 + 3 + 1 + 3 + 1 + 3 + 1 + 10 + 1 + 1];
char         *p, *pNext;
__be32       addr = 0;
unsigned int i, j;
unsigned int val;
char         end;

  if (count >= sizeof(cnf))
  {
    return -EINVAL;
  }

  if (copy_from_user(cnf, buf, count))
  {
    return -EFAULT;
  }

  // Remove the ending EOL if any
  if ('\n' == cnf[count - 1])
  {
    cnf[count - 1] = '\0';
  }
  else
  {
    cnf[count] = '\0';
  }

  p = cnf;
  end = '.';
  val = 0;
  for (j = 0; j < 4; j ++)
  {
    for (i = 0; i < 4; i ++)
    {
      if (end == p[i])
      {
        p[i] = '\0';
        pNext = &(p[i]) + 1;
        break;
      }

      if ('\0' == p[i])
      {
        if (3 == j)
	{
          pNext = &(p[i]);
          break;
	}
        else
	{
          return -EINVAL;
	}
      }

      if ((p[i] < '0') || (p[i] > '9'))
      {
        return -EINVAL;
      }

      val = val * 10 + p[i] - '0';
    } // End for

    if (4 == i)
    {
      return -EINVAL;
    }

    if (val > 255)
    {
      return -EINVAL;
    }
    addr |= (val << ((3 - j) * 8));

    if (2 == j)
    {
      end = ':';
    } 

    p = pNext;
    val = 0;
  } // End for

  // Get the port number
  if (*p)
  {
    i = 0;
    while (p[i])
    {
      if (p[i] < '0' || p[i] > '9')
      {
        return -EINVAL;
      }

      val = val * 10 + p[i] - '0';
      i ++;
    } // End while

    udpc_port = val;
  }

  udpc_addr = addr;

  UDPC_PRINT(KERN_INFO, "New destination is %u.%u.%u.%u:%u\n"
                      ,
	              udpc_addr >> 24 & 0xFF, udpc_addr >> 16 & 0xFF, udpc_addr >> 8 & 0xFF, udpc_addr & 0xFF,
                      udpc_port);

  return count;
} /* udpc_proc_write */


const struct file_operations udpc_proc_ops =
{
  .read  = udpc_proc_read,
  .write = udpc_proc_write,
};



// ----------------------------------------------------------------------------
// Name        : udpc_init
// Description : Entry point of the driver
// Return      : 0, if OK
//               -errno, if error
// ----------------------------------------------------------------------------
static int __init udpc_init(void)
{
  /* Some initializations first */
  memset(&udpc_thd, 0, sizeof(struct udpc_thd_info));
  init_completion(&(udpc_thd.terminated));

  /* Create the /proc entry */
  udpc_proc = proc_create(UDPC_MODULE_NAME, S_IRUSR | S_IWUSR, NULL, &udpc_proc_ops);
  if (NULL == udpc_proc)
  {
    UDPC_PRINT(KERN_ERR, "Unable to create the /proc/%s entry\n", UDPC_MODULE_NAME);
    return -EIO;
  }

  UDPC_PRINT(KERN_DEBUG, "Starting the kernel thread\n");

  /* Start the kernel thread */
  udpc_thd.thread_id = kthread_run((void *)udpc_thd_entry, NULL, UDPC_MODULE_NAME);
  if (IS_ERR(udpc_thd.thread_id)) 
  {
    UDPC_PRINT(KERN_ERR, "Unable to start the kernel thread\n");
    return -ENOMEM;
  }

  UDPC_PRINT(KERN_INFO, "Registering to console subsystem\n");

  /* Register to the console subsystem */
  register_console(&udpc_console);

  UDPC_PRINT(KERN_INFO, "Destination is %u.%u.%u.%u:%u\n"
                      ,
	              udpc_addr >> 24 & 0xFF, udpc_addr >> 16 & 0xFF, udpc_addr >> 8 & 0xFF, udpc_addr & 0XFF,
                      udpc_port);

  return 0;
} /* udpc_init */


// ----------------------------------------------------------------------------
// Name        : udpc_exit
// Description : Exit point of the driver
// ----------------------------------------------------------------------------
static void __exit udpc_exit(void)
{
int rc;

  UDPC_PRINT(KERN_INFO, "Unregistering from console subsystem\n");

  /* This call returns 1, if the console has not been found. 0, otherwise */
  (void)unregister_console(&udpc_console);

  /* If the thread is running */
  if (udpc_thd.thread_id)
  {
    lock_kernel();

    rc = kill_pid(udpc_thd.pid, SIGKILL, 1);
    put_pid(udpc_thd.pid);

    unlock_kernel();

    /* Wait for kernel thread to die */
    if (rc < 0)
    {
      UDPC_PRINT(KERN_INFO, "Unknown error %d while trying to terminate kernel thread\n", -rc);
      return;
    }
    else 
    {
      wait_for_completion(&(udpc_thd.terminated));

      UDPC_PRINT(KERN_DEBUG, "Succesfully killed kernel thread\n");
    }
  } /* End if thread is running */

  /* Free allocated resources before exit */
  if (udpc_thd.sock_send) 
  {
    sock_release(udpc_thd.sock_send);
    udpc_thd.sock_send = NULL;
  }

  /* If the /proc entry exists */
  if (udpc_proc)
  {
    remove_proc_entry(UDPC_MODULE_NAME, NULL);
    udpc_proc = NULL;
  }

  UDPC_PRINT(KERN_DEBUG, "Module unloaded\n");
} /* udpc_exit */



/* Initialization and cleanup functions */
module_init(udpc_init);
module_exit(udpc_exit);


/* Module information */
MODULE_DESCRIPTION("UDP based console");
MODULE_AUTHOR("Rachid Koucha <rachid dot koucha at free dot fr>");
MODULE_LICENSE("GPL");

