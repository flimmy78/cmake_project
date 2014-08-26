#
#  Copyright (C) 2007 Rachid Koucha <rachid dot koucha at free dot fr>
#
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#


# Copy the files to the destination directory
EXECUTE_PROCESS(COMMAND chown root ${CMAKE_INSTALL_PREFIX}/bin/pdip
                COMMAND chgrp root ${CMAKE_INSTALL_PREFIX}/bin/pdip
                COMMAND chown root ${CMAKE_INSTALL_PREFIX}/share/man/fr/man1/pdip.1.gz
                COMMAND chgrp root ${CMAKE_INSTALL_PREFIX}/share/man/fr/man1/pdip.1.gz
                COMMAND chown root ${CMAKE_INSTALL_PREFIX}/share/man/man1/pdip.1.gz
                COMMAND chgrp root ${CMAKE_INSTALL_PREFIX}/share/man/man1/pdip.1.gz
               )


