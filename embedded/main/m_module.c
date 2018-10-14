#include <stdint.h>

#include "m_module.h"

/* Global module list that is used to track installed modules */
m_init_t module_list[MODULE_MAX_COUNT] = {0};
m_init_t (*module_init_funcs[MODULE_MAX_COUNT])(void);
uint8_t module_count = 0;

m_init_t* module_start(void)
{
	for(uint8_t i = 0; i < module_count; i++)
	{
		module_list[i] = module_init_funcs[i]();
	}

	return module_list;
}

int module_register(m_init_t (*func)(void))
{
	int ret = -1;

	if(module_count < MODULE_MAX_COUNT)
	{
		module_list[module_count] = M_INIT_NOT;
		module_init_funcs[module_count] = func;

		ret = module_count++;
	}

	return ret;
}

void module_init(void)
{
	// don't need to do anything here quite yet
	return;
}