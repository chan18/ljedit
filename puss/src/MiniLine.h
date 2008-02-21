// MiniLine.h
// 

#ifndef PUSS_INC_MINILINE_H
#define PUSS_INC_MINILINE_H

struct Puss;
struct MiniLine;
struct MiniLineCallback;

void	puss_mini_line_create(Puss* app);
void	puss_mini_line_destroy(Puss* app);

void	puss_mini_line_active(Puss* app, MiniLineCallback* cb);
void	puss_mini_line_deactive(Puss* app);

#endif//PUSS_INC_MINILINE_H

