#include <kservice.h>
#include <kmimetype.h>
#include <kservicetype.h>

#include <kcmdlineargs.h>
#include <kapplication.h>
#include <kdebug.h>

int main(int argc, char *argv[])
{
    KCmdLineArgs::init( argc,argv, "getalltest", 0, ki18n("getalltest"), 0);
    KApplication k( false /*noGUI*/); // KMessageBox needs KApp for makeStdCaption

//for (int i = 0 ; i < 2 ; ++i ) { // test twice to see if they got deleted
   kDebug() << "All services";
   KService::List services = KService::allServices();
   kDebug() << "got " << services.count() << " services";
   KService::List::const_iterator s = services.begin();
   for ( ; s != services.end() ; ++s )
   {
     kDebug() << (*s)->name() << " " << (*s)->entryPath();
   }
//}

   kDebug() << "All mimeTypes";
   KMimeType::List mimeTypes = KMimeType::allMimeTypes();
   kDebug() << "got " << mimeTypes.count() << " mimeTypes";
   KMimeType::List::const_iterator m = mimeTypes.begin();
   for ( ; m != mimeTypes.end() ; ++m )
   {
     kDebug() << (*m)->name();
   }

   kDebug() << "All service types";
   KServiceType::List list = KServiceType::allServiceTypes();
   kDebug() << "got " << list.count() << " service types";
   KServiceType::List::const_iterator st = list.begin();
   for ( ; st != list.end() ; ++st )
   {
     kDebug() << (*st)->name();
   }

   kDebug() << "done";

   return 0;
}
