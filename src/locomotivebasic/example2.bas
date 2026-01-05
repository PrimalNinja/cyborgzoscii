10 MEMORY &3FFF:MODE 1:DEFINT a-z:RANDOMIZE TIME
20 DIM c(255),ad(255,15):REM 16 instances per byte
30 raw=&4000:REM 2KB ROM starts here
40 PRINT "--- ZOSCII 2D CPC ENGINE ---"
50 PRINT "1. GENERATING 2KB ENTROPY..."
60 FOR i=0 TO 2047:b=INT(RND*256):POKE raw+i,b:NEXT
70 PRINT "2. INDEXING TO 2D ARRAY..."
80 FOR i=0 TO 2047
90   by=PEEK(raw+i)
100  IF c(by)<16 THEN ad(by,c(by))=i:c(by)=c(by)+1
110 NEXT
120 PRINT "3. LINK READY. $I(M;A)=0$"
130 REM --- Main Interactive Loop ---
140 PRINT:INPUT "MSG: ",m$:IF m$="" THEN END
150 L=LEN(m$):DIM en(L):REM Redimensioned per message
160 PRINT "ENCODED: ";
170 FOR n=1 TO L:by=ASC(MID$(m$,n,1))
180   IF c(by)=0 THEN PRINT "SKIP:";CHR$(by);:GOTO 210
190   slot=INT(RND*c(by))
200   en(n)=ad(by,slot):PRINT HEX$(en(n),3);" ";
210 NEXT:PRINT
220 PRINT "DECODED: ";
230 FOR n=1 TO L:PRINT CHR$(PEEK(raw+en(n)));:NEXT
240 PRINT:ERASE en:GOTO 140