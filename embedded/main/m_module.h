#ifndef _M_MODULE_H
#define _M_MODULE_H

#define MODULE_MAX_COUNT 6
#define NO_MODULE 255;

typedef uint8_t m_module_t;

typedef enum {
	M_INIT_NONE = 0,
	M_INIT_NOT,
	M_INIT_OK,
	M_INIT_ERROR
} m_init_t;

/** 
 *	@brief Initializes the module extension framework
 */
void module_init(void);

#endif