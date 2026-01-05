10 MEMORY &3FFF:MODE 1:DEFINT a-z:RANDOMIZE TIME
20 DIM c(255),ad(255,7):REM DIM once at start to avoid Error 10
30 raw=&4000:pro=&4800
40 PRINT "1. FILLING RAW ROM..."
50 FOR i=0 TO 2047:POKE raw+i,INT(RND*256):NEXT
60 PRINT "2. PROCESSING (PRO-ROM)..."
70 FOR i=0 TO 2047
80   by=PEEK(raw+i)
90   IF c(by)<8 THEN ad(by,c(by))=i:c(by)=c(by)+1
100 NEXT
110 PRINT "3. SYSTEM READY."
120 PRINT:INPUT "MSG: ",m$:IF m$="" THEN END
130 L=LEN(m$):PRINT "ENCODED: ";
140 FOR n=1 TO L:by=ASC(MID$(m$,n,1))
150   REM --- Only pick from slots we actually filled ---
160   IF c(by)=0 THEN PRINT "ERR: NO ROM COVERAGE":GOTO 120
170   slot=INT(RND*c(by))
180   en=ad(by,slot)
190   PRINT hex$(en,3);" ";:REM Show 3-hex digit offset
200   REM Store encoded stream in a temporary array for decoding
210   POKE &3F00+n, en AND 255: POKE &3F80+n, en\256
220 NEXT:PRINT
230 PRINT "DECODING: ";
240 FOR n=1 TO L
250   en=PEEK(&3F00+n) + PEEK(&3F80+n)*256
260   PRINT CHR$(PEEK(raw+en));
270 NEXT:PRINT:GOTO 120