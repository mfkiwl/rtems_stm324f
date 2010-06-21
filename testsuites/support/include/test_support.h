/*
 *  COPYRIGHT (c) 1989-2009.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id$
 */

#ifndef __TEST_SUPPORT_h
#define __TEST_SUPPORT_h

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Allocate a number of small blocks from the heap
 *  until the largest free block of memory available is
 *  smaller than smallest.
 *
 *  NOTE: The memory CANNOT be freed.
 */
void Allocate_majority_of_workspace( int smallest );

/*
 *  Return a pointer to the POSIX name that is slightly
 *  beyond the legal limit.
 */
const char *Get_Too_Long_Name(void);

/*
 *  Return a pointer to the longest legal POSIX name.
 */
const char *Get_Longest_Name(void);

/*
 *  Spin for specified number of ticks.
 */
void rtems_test_spin_for_ticks(int ticks);

/*
 *  Spin until the next clock tick
 */
void rtems_test_spin_until_next_tick( void );

/*********************************************************************/
/*********************************************************************/
/**************              TMTEST SUPPORT             **************/
/*********************************************************************/
/*********************************************************************/

/*
 *  Type of method used for timing operations
 */
typedef void (*rtems_time_test_method_t)(
  int    iteration,
  void  *argument
);

/*
 *  Obtain baseline timing information for benchmark tests.
 */
void rtems_time_test_measure_operation(
  const char               *description,
  rtems_time_test_method_t  operation,
  void                     *argument,
  int                       iterations,
  int                       overhead
);

#ifdef __cplusplus
};
#endif

#endif
