#include "fcaseopen.h"

// based on https://github.com/OneSadCookie/fcaseopen

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>

// r must have strlen(path) + 3 bytes
static int casepath(char const *path, char *r)
{
    // TODO: C++
    size_t l = strlen(path);
    char *p = (char*)alloca(l + 1);
    strcpy(p, path);
    size_t rl = 0;
    
    DIR *d;
    if (p[0] == '/')
    {
        d = opendir("/");
        p = p + 1;
    }
    else
    {
        d = opendir(".");
        r[0] = '.';
        r[1] = 0;
        rl = 1;
    }
    
    int last = 0;
    char *c = strsep(&p, "/");
    while (c)
    {
        if (!d)
        {
            return 0;
        }
        
        if (last)
        {
            closedir(d);
            return 0;
        }
        
        r[rl] = '/';
        rl += 1;
        r[rl] = 0;
        
        struct dirent *e = readdir(d);
        while (e)
        {
            if (strcasecmp(c, e->d_name) == 0)
            {
                strcpy(r + rl, e->d_name);
                rl += strlen(e->d_name);

                closedir(d);
                d = opendir(r);
                
                break;
            }
            
            e = readdir(d);
        }
        
        if (!e)
        {
            strcpy(r + rl, c);
            rl += strlen(c);
            last = 1;
        }
        
        c = strsep(&p, "/");
    }
    
    if (d) closedir(d);
    return 1;
}

FILE *fcaseopen(char const *path, char const *mode)
{
    FILE *f = fopen(path, mode);
    if (!f)
    {
        char *r = (char*)alloca(strlen(path) + 3);
        if (casepath(path, r))
        {
            f = fopen(r, mode);
        }
    }
    return f;
}

DIR* caseopendir(const char* path)
{
    DIR *d = opendir(path);
    if (!d)
    {
        char *r = (char*)alloca(strlen(path) + 3);
        if (casepath(path, r))
        {
            d = opendir(r);
        }
    }
    return d;
}

int casestat(const char* pathname, struct stat *statbuf) {

    // TODO: C++
    int res = stat(pathname, statbuf);
    if (res) {
        char *r = (char*)alloca(strlen(pathname) + 3);
        if (casepath(pathname, r))
        {
            res = stat(r, statbuf);
        }
    }
    return res;
}