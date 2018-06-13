#include <my_app.h>
#include <my_mesh.h>
#include <version.h>

void main(void)
{
	printk("Firmware version: v%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR,
			VERSION_BUILD);

	my_app_init();
	my_mesh_init();
}
