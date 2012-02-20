/* Asynchronous Database Access With Qt 

  Example program. See db.h to change database connection parameters.

*/

#include <QApplication>
#include "win_impl.h"

int main( int argc, char **argv )
{
  QApplication app( argc, argv );
  Win win;
  win.show();
  return app.exec();
}
