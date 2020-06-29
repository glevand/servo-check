/*
 * Moving average filter.
 * 
 */

#if !defined(_FILTER_H)
#define _FILTER_H

struct ave_filter;
struct ave_filter *ave_filter_init(int len);
void ave_filter_delete(struct ave_filter *f);
int ave_filter_run(struct ave_filter *f, int x);

#endif /* _FILTER_H */
