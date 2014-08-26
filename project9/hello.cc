#include <QtGui/QApplication>
#include <QtGui/QLabel>

int main(int ac, char *av[])
{
  QApplication app(ac, av);
  QLabel *label = new QLabel("<h1>Hello Qt!</h1>");
  label->show();
  return app.exec();
}
