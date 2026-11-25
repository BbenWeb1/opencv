/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Bben                                                         |
   +----------------------------------------------------------------------+
*/

// php_opencv.h — عام
#ifndef PHP_OPENCV_H
# define PHP_OPENCV_H

# ifdef __cplusplus
extern "C" {
# endif

#include "php.h"
#include "ext/standard/info.h"

# ifdef __cplusplus
}
# endif

extern zend_module_entry opencv_module_entry;
# define phpext_opencv_ptr &opencv_module_entry
# define PHP_OPENCV_VERSION "0.2.0"

# if defined(ZTS) && defined(COMPILE_DL_OPENCV)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif
