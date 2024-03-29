/************************************************************** 
* FILE:         PWM_drv.c
* DESCRIPTION:  A/D in PWM driver for TMS320F2808
* AUTHOR:       Andra� Kontar�ek, Mitja Nemec
*
****************************************************************/
#include "PWM_drv.h"

// prototipi lokalnih funkcij


/**************************************************************
* Funkcija, ki popi�e registre za PWM1,2 in 3. Znotraj funkcije
* se omogo�i interrupt za pro�enje ADC, popi�e se perioda, compare
* register, tripzone register, omogo�i se izhode za PWM...
* return:void
**************************************************************/
void PWM_init(void)
{
//EPWM Module 1
    // setup timer base 
    EPwm7Regs.TBPRD = PWM_PERIOD/2;       //nastavljeno na 25us, PWM_PERIOD = 50us
    EPwm7Regs.TBCTL.bit.PHSDIR = 0;       // count up after sync
    EPwm7Regs.TBCTL.bit.CLKDIV = 0;
    EPwm7Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm7Regs.TBCTL.bit.SYNCOSEL = 1;     // sync out on zero
    EPwm7Regs.TBCTL.bit.PRDLD = 0;        // shadowed period reload
    EPwm7Regs.TBCTL.bit.PHSEN = 0;        // master timer does not sync
    EPwm7Regs.TBCTR = 1;

        // debug mode behafiour
    #if PWM_DEBUG == 0
    EPwm7Regs.TBCTL.bit.FREE_SOFT = 0;  // stop immediately
    EPwm7Regs.TBCTL.bit.FREE_SOFT = 0;  // stop immediately
    #endif
    #if PWM_DEBUG == 1
    EPwm7Regs.TBCTL.bit.FREE_SOFT = 1;  // stop when finished
    EPwm7Regs.TBCTL.bit.FREE_SOFT = 1;  // stop when finished
    #endif
    #if PWM_DEBUG == 2
    EPwm7Regs.TBCTL.bit.FREE_SOFT = 3;  // run free
    EPwm7Regs.TBCTL.bit.FREE_SOFT = 3;  // run free
    #endif
    
    // Compare registers
    EPwm7Regs.CMPA.half.CMPA = PWM_PERIOD/4;                 //50% duty cycle

    // Init Action Qualifier Output A Register 
    EPwm7Regs.AQCTLA.bit.CAU = AQ_CLEAR;  // clear output on CMPA_UP
    EPwm7Regs.AQCTLA.bit.CAD = AQ_SET;    // set output on CMPA_DOWN

    // Dead Time
    
    // Trip zone 

    // Event trigger
    // Pro�enje ADC-ja
    EPwm7Regs.ETSEL.bit.SOCASEL = 2;    //spro�i prekinitev na periodo
    EPwm7Regs.ETPS.bit.SOCAPRD = 1;     //ob vsakem prvem dogodku
    EPwm7Regs.ETCLR.bit.SOCA = 1;       //clear possible flag
    EPwm7Regs.ETSEL.bit.SOCAEN = 1;     //enable ADC Start Of conversion
    // Pro�enje prekinitve 
    EPwm7Regs.ETSEL.bit.INTSEL = 2;             //spro�i prekinitev na periodo
    EPwm7Regs.ETPS.bit.INTPRD = PWM_INT_PSCL;   //ob vsakem prvem dogodku
    EPwm7Regs.ETCLR.bit.INT = 1;                //clear possible flag
    EPwm7Regs.ETSEL.bit.INTEN = 1;              //enable interrupt


    // output pin setup
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO40 = 1;   // GPIO40 pin is under ePWM control
    EDIS;                                 // Disable EALLOW

}   //end of PWM_PWM_init

/**************************************************************
* Funkcija, ki na podlagi vklopnega razmerja in izbranega vektorja
* vklopi dolo�ene tranzistorje
* return: void
* arg1: vklopno razmerje od 0.0 do 1.0 (format IQ)
**************************************************************/
void PWM_update(float duty)
{
   int compare;

    // za��ita za duty cycle 
    //(za��ita za sektor je narejena v default switch case stavku)
    if (duty < 0.0) duty = 0.0;
    if (duty > 1.0) duty = 1.0;

    //izra�unam vrednost compare registra(iz periode in preklopnega razmerja)
    compare = (int)((PWM_PERIOD/2) * duty);

    // vpisem vrednost v register
    EPwm7Regs.CMPA.half.CMPA = compare;
    

}  //end of AP_PWM_update

/**************************************************************
* Funkcija, ki nastavi periodo, za doseganje zeljene periode
* in je natancna na cikel natancno
* return: void
* arg1: zelena perioda
**************************************************************/
void PWM_period(float perioda)
{
    // spremenljivke
    float   temp_tbper;
    static float ostanek = 0;
    long celi_del;

    // naracunam TBPER (CPU_FREQ * perioda)
    temp_tbper = perioda * CPU_FREQ/2;
    
    // izlocim celi del in ostanek
    celi_del = (long)temp_tbper;
    ostanek = temp_tbper - celi_del;
    // povecam celi del, ce je ostanek veji od 1
    if (ostanek > 1.0)
    {
        ostanek = ostanek - 1.0;
        celi_del = celi_del + 1;
    }
    
    // nastavim TBPER
    EPwm7Regs.TBPRD = celi_del;
}   //end of FB_period

/**************************************************************
* Funkcija, ki nastavi periodo, za doseganje zeljene frekvence
* in je natancna na cikel natancno
* return: void
* arg1: zelena frekvenca
**************************************************************/
void PWM_frequency(float frekvenca)
{
    // spremenljivke
    float   temp_tbper;
    static float ostanek = 0;
    long celi_del;

    // naracunam TBPER (CPU_FREQ / SAMPLING_FREQ) - 1
    temp_tbper = (CPU_FREQ/2) / frekvenca;

    // izlocim celi del in ostanek
    celi_del = (long)temp_tbper;
    ostanek = temp_tbper - celi_del;
    // povecam celi del, ce je ostanek veji od 1
    if (ostanek > 1.0)
    {
        ostanek = ostanek - 1.0;
        celi_del = celi_del + 1;
    }
    
    // nastavim TBPER
    EPwm7Regs.TBPRD = celi_del - 1;
}   //end of FB_frequency
  
/**************************************************************
* Funkcija, ki starta PWM1. Znotraj funkcije nastavimo
* na�in �tetja �asovnikov (up-down-count mode)
* return: void
**************************************************************/
void PWM_start(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EPwm7Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;  //up-down-count mode
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
    
}   //end of AP_PWM_start



