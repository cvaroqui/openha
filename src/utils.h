#include <time.h>
#include <cluster.h>

gboolean
init_var()
{
	if ((EZ = getenv("EZ")) == NULL) {
		printf("Error: variable EZ not defined\n");
		return FALSE;
	}
	if ((EZ_BIN = getenv("EZ_BIN")) == NULL) {
		printf("Error: variable EZ_BIN not defined\n");
		return FALSE;
	}
	if ((EZ_MONITOR = getenv("EZ_MONITOR")) == NULL) {
		printf("Error: variable EZ_MONITOR not defined\n");
		return FALSE;
	}
	if ((EZ_SERVICES = getenv("EZ_SERVICES")) == NULL) {
		printf("Error: variable EZ_SERVICES not defined\n");
		return FALSE;
	}
	if ((EZ_LOG = getenv("EZ_LOG")) == NULL) {
		printf("Error: variable EZ_LOG not defined\n");
		return FALSE;
	}
	if ((EZ_NODES = getenv("EZ_NODES")) == NULL) {
		printf("Error: variable EZ_NODES not defined\n");
		return FALSE;
	}
	return TRUE;
}
