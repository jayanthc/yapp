**************************************************************************
***  THIS PROGRAM GENERATE A TABLE OF ERROR-FUNCTION VALUES
***  FOR THE ARGUMENT(OF ERROR-FUNCTION) RANGE FROM
***  0.0 TO ( 10.0/SQRT(2) )
**************************************************************************
***  CORRESPONDING VALUES OF ARGUMENTS ARE WRITTEN IN SECOND COLUMN OF THE
***  TABLE AND THOSE OF THRESHOLDS (HOW MANY SIGMA'S IN PROBABILITY 
***  DISTRIBUTION), AT WHICH THESE VALUES OF ERROR-FUNCTION ARE EVALUATED,
***  ARE ALSO WRITTEN IN THE THIRD COLUMN OF THE TABLE
**************************************************************************
***  FOR CONVENIENCE COLUMN HEADERS ARE GIVEN AT THE END OF THE TABLE
**************************************************************************

          IMPLICIT NONE
          INTEGER*4 I,J,N
cc          DOUBLE PRECISION   ERRF,Z,RANG,TERM,PI,SQRT2,DIFF,THRESH
          REAL*8  ERRF,Z,RANG,TERM,PI,SQRT2
          REAL*8  DIFF,THRESH,MIN_DIFF

          OPEN(UNIT=11,FILE='ERF_LOOKUP_TABLE',STATUS='UNKNOWN')
 
          PI = ATAN(1.) * 4.   ! acos(-1.0)
          SQRT2 = SQRT(2.d0)
          DO N = 1,1000
             Z=dble(N-1)/(100.d0*SQRT2)
             TERM = ERRF(Z)
             WRITE(11,'(3(1x,f16.12))')TERM,Z,dble(N-1)/100.d0
          END DO
          WRITE(11,'(3(1X,A16))')'ERR(X/SQRT(2))','X/SQRT(2)','( X )'
          STOP
          END
*****************************************************************
          FUNCTION ERRF(X) 
          IMPLICIT NONE
cc          DOUBLE PRECISION   ERRF,X,P,T,A1,A2,A3,A4,A5
          REAL*8   ERRF,X,P,T,A1,A2,A3,A4,A5
          
          P = 0.3275911d0
          A1 = 0.254829592d0
          A2 = -0.284496736d0
          A3 = 1.421413741d0 
          A4 = -1.453152027d0
          A5 = 1.061405429d0
          T = 1.d0/(1.d0 + P*X)

          ERRF = 1.d0 - ( A1*T + A2*T*T + A3*T*T*T + A4*T*T*T*T +
     +                     A5*T*T*T*T*T ) * EXP(-X*X)

CC        FORMULA NO. 7.1.26 FROM "HANDBOOK OF MATHEMATICAL FUNCTIONS"
CC        BY ABRAMOWITZ AND STEGUN HAS BEEN USED HERE

          RETURN
          END
*********************************************************************
