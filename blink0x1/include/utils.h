#ifndef	_UTILS_H
#define	_UTILS_H

extern void delay ( unsigned long);
extern void put32 ( unsigned long, unsigned int );
extern void putA32 ( unsigned long, unsigned int* );
extern unsigned int get32 ( unsigned long );
extern unsigned int* getA32 ( unsigned long );
extern int get_el ( void );
extern unsigned int getCore();
extern void hang();

extern void dsb();
extern void dmb();
extern void isb();
extern void burn();
// extern void write_fence(unsigned *);
// extern void read_fence(unsigned *);

#endif  /*_UTILS_H */
