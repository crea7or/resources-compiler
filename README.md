# Resources compiler can be used to compile binary files into an executable with easy access from code

use this to generate resources:
```
resources_compiler --sources=file1,file2 --output=full/path/to/file_without_extension
```

without supplying --sources param, an empty resources holder classes will be generated
note: spaces are not allowed in paths nor between the tags nor the equal signs!


use this to access resources from code:
```
#include "full/path/to/file_without_extension.h"

...

const resources::manager resources;
const auto resource_file = resources.get("file1");
if (nullptr != resource_file.data()) {
	// resource exist
} else {
	// no such resource
}
```