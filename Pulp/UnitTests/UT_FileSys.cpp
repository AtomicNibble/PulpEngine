#include "stdafx.h"
#include <IFileSys.h>

X_USING_NAMESPACE;

using namespace core;



// Want to test hte virtual filesystem.
// And handling of unicode.
// The file system only supports asci for relative paths.
// but the engine can have a working dir with unicode charaters.
// Tool also need to be able to access absolute unicode paths.
// Don't think tools will need to support relative unicode paths.


TEST(FileSys, Write)
{



}