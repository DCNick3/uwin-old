
#include "uwin/util/file.h"
#include <stdio.h>
#include <stdlib.h>

bool uw_test_file(const char* path, int test) {
    switch (test) {
        case UW_TEST_FILE_EXISTS:
        {
            // the most portable (c) (and definetely not the best) way
            FILE* f = fopen(path, "rb");
            if (f == NULL)
                return false;
            fclose(f);
            return true;
        }
        
        default:
            abort();
    }
}
