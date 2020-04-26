#ifndef __HDLC_FUNC_H__
#define __HDLC_FUNC_H__

void setControl(Control &control);
void splitInfo(char *msg, char *buffer， int dest);
int I_Framing(char *info_beg, char *info_end, char *buffer, int dest);

#endif