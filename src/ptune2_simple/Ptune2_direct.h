/*
===============================================================================

 Ptune2 is SH7305 CPG&BSC tuning utility for PRIZM fx-CG10/20  v1.10

 copyright(c)2014,2015,2016 by sentaro21
 e-mail sentaro21@pm.matrix.jp

===============================================================================
*/

// All Ptune functions should be avoided on CG50 devices until proven functional!
// setting values
#define PT2_DEFAULT 0			// 59 Mhz, default settings
#define PT2_NOMEMWAIT 1			// 59 Mhz, no mem waits
#define PT2_HALFINC	2			// 97 MHz, 150% speed optimized
#define PT2_DOUBLE 3			// 118 MHz, 200% speed optimized
#define PT2_UNKNOWN 4			// unknown setting, used for settings that are not the above for Ptune2_GetSetting

typedef struct {
	unsigned int regs[8];
} PTuneRegs;

//---------------------------------------------------------------------------------------------
void Ptune2_LoadSetting(int setting);	// loads setting based on defines above
int Ptune2_GetSetting();				// returns the current CPU setting as defined above

void Ptune2_LoadBackup();				//  backup     -> CPG Register
void Ptune2_SaveBackup();				//  CPG Register -> backup

PTuneRegs Ptune2_GetRegs();				// returns current cpu clock regs directly

