/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYSCALLSSIGNALMOCKDEFINES_H_
#define SCORE_TIME_SYSCALLSSIGNALMOCKDEFINES_H_

#define signal MockSignal
#define raise MockRaise

int MockRaise(int signum);
int MockSignal(int signum, void(*handler)(int));

#endif // SCORE_TIME_SYSCALLSSIGNALMOCKDEFINES_H_
