#include <stdlib.h>
#include <sys/stat.h>
#include <cf3.defs.h>
#include <dbm_api.h>
#include <lastseen.h>
#include <known_dirs.h>

char CFWORKDIR[CF_BUFSIZE];

static void tests_setup(void)
{
    static char env[] = /* Needs to be static for putenv() */
        "CFENGINE_TEST_OVERRIDE_WORKDIR=/tmp/lastseen_migration_test.XXXXXX";

    char *workdir = strchr(env, '=') + 1; /* start of the path */
    assert(workdir - 1 && workdir[0] == '/');

    mkdtemp(workdir);
    strlcpy(CFWORKDIR, workdir, CF_BUFSIZE);
    putenv(env);
    mkdir(GetStateDir(), (S_IRWXU | S_IRWXG | S_IRWXO));
}

void UpdateLastSawHost(const char *hostkey, const char *address,
                       bool incoming, time_t timestamp);

int main()
{
    tests_setup();

    for (int i = 0; i < 1000000; ++i)
    {
        if ((i % 10000) == 0)
        {
            printf(".");
            fflush(stdout);
        }

        char hostkey[50];
        xsnprintf(hostkey, 50, "SHA-%040d", i);
        char ip[50];
        xsnprintf(ip, 50, "250.%03d.%03d.%03d", i / (256*256), (i / 256) % 256, i % 256);

        UpdateLastSawHost(hostkey, ip, false, i);
        UpdateLastSawHost(hostkey, ip, true, 2000000 - i);
    }

    char cmd[CF_BUFSIZE];
    xsnprintf(cmd, CF_BUFSIZE, "rm -rf '%s'", CFWORKDIR);
    system(cmd);

    return 0;
}

/* STUBS */

void FatalError(char *s, ...)
{
    exit(42);
}

HashMethod CF_DEFAULT_DIGEST;
const char *const DAY_TEXT[] = {};
const char *const MONTH_TEXT[] = {};
const char *const SHIFT_TEXT[] = {};
pthread_mutex_t *cft_output;
char VIPADDRESS[CF_MAX_IP_LEN];
RSA *PUBKEY;

Item *IdempPrependItem(Item **liststart, const char *itemstring, const char *classes)
{
    exit(42);
}

bool IsItemIn(Item *list, const char *item)
{
    exit(42);
}

void DeleteItemList(Item *item)
{
    exit(42);
}

bool MINUSF;

char *MapAddress(char *addr)
{
    exit(42);
}

char *HashPrintSafe(char *dst, size_t dst_size, const unsigned char *digest,
                    HashMethod type, bool use_prefix)
{
    exit(42);
}

void HashPubKey(RSA *key, unsigned char digest[EVP_MAX_MD_SIZE + 1], HashMethod type)
{
    exit(42);
}

void *ConstraintGetRvalValue(char *lval, Promise *promise, char type)
{
    exit(42);
}
