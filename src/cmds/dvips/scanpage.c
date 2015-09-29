/*
 *   This routine scans a page for font definitions and character usage.
 *   It keeps an estimate of the memory remaining in the printer, and
 *   returns false when that memory is exhausted, unless scanning the
 *   first page. Otherwise it returns 1 if no problems were found,
 *   or 2 if it thinks the first page of a section may overflow memory.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines we need.
 */
extern shalfword dvibyte() ;
extern halfword twobytes() ;
extern integer threebytes() ;
extern integer signedquad() ;
extern shalfword signedbyte() ;
extern shalfword signedpair() ;
extern integer signedtrio() ;
extern void skipover() ;
extern void fontdef() ;
extern void predospecial() ;
extern Boolean residentfont() ;
extern Boolean virtualfont() ;
extern void loadfont() ;
extern void error() ;
extern long getlong() ;
/*
 *   And, of course, the globals it uses.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern fontdesctype *curfnt ;
extern fontmaptype *ffont ;
extern quarterword *curpos, *curlim ;
extern integer fontmem ;
extern integer pagecount ;
extern integer pagenum ;
extern Boolean compressed ;
extern char errbuf[] ;
extern frametype frames[] ;
/*
 *   Now our scanpage routine.
 */
short
scanpage()
{
   register shalfword cmd ;
   register chardesctype *cd ;
   register quarterword *cras ;
   register long thischar ;
   register integer instring = 0 ;
   register fontmaptype *cfnt ;
   integer fnt ;
   register long pagecost = 0 ;
   register frametype *frp = frames ;

#ifdef DEBUG
   if (dd(D_PAGE))
#ifdef SHORTINT
   (void)fprintf(stderr,"Scanning page %ld\n", pagenum) ;
#else   /* ~SHORTINT */
   (void)fprintf(stderr,"Scanning page %d\n", pagenum) ;
#endif  /* ~SHORTINT */
#endif  /* DEBUG */
   curfnt = NULL ;
   while (1) {
      switch (cmd=dvibyte()) {
case 129: case 130: case 131: case 133: case 134: case 135: case 136:
case 139: case 236: case 237: case 238: case 244: case 245: case 246:
case 247: case 248: case 249: case 250: case 251: case 252: case 253:
case 254: case 255: /* unimplemented or illegal commands */
         (void)sprintf(errbuf,
            "! DVI file contains unexpected command (%d)",cmd) ;
         error(errbuf) ;
case 132: case 137: /* eight-byte commands setrule, putrule */
         (void)dvibyte() ;
         (void)dvibyte() ;
         (void)dvibyte() ;
         (void)dvibyte() ;
case 146: case 151: case 156: case 160: case 165: case 170:
   /* four-byte commands right4, w4, x4, down4, y4, z4 */
         (void)dvibyte() ;
case 145: case 150: case 155: case 159: case 164: case 169:
   /* three-byte commands right3, w3, x3, down3, y3, z3 */
         (void)dvibyte() ;
case 144: case 149: case 154: case 158: case 163: case 168:
   /* two-byte commands right2, w2, x2, down2, y2, z2 */
         (void)dvibyte() ;
case 143: case 148: case 153: case 157: case 162: case 167:
   /* one-byte commands right1, w1, x1, down1, y1, z1 */
         (void)dvibyte() ;
case 147: case 152: case 161: case 166: /* w0, x0, y0, z0 */
         instring = 0 ;
case 138: case 141: case 142: /* nop, push, pop */
         break ;
case 128: cmd = dvibyte() ; /* set1 commands drops through to setchar */
default:    /* these are commands 0 (setchar0) thru 127 (setchar 127) */
/*
 *   We are going to approximate that each string of consecutive characters
 *   requires STRINGCOST + (length of string) bytes of PostScript VM.
 */
         if (curfnt==NULL)
            error("! Bad DVI file: no font selected") ;
         if (curfnt->loaded == 2) { /* scanning a virtual font character */
            frp->curp = curpos ;
            frp->curl = curlim ;
            frp->ff = ffont ;
            frp->curf = curfnt ;
            if (++frp == &frames[MAXFRAME] )
               error("! virtual recursion stack overflow") ;
            cd = curfnt->chardesc + cmd ;
            curpos = cd->packptr + 2 ;
            curlim = curpos + (256*(long)(*cd->packptr)+(*(cd->packptr+1))) ;
            ffont = curfnt->localfonts ;
            if (cfnt = ffont)
               goto fontfound ; /* select the first local font */
            curfnt = NULL ; /* unless there isn't one */
         } else {
            pagecost++ ;
            if (! instring) {
               pagecost += STRINGCOST ;
               instring = 1 ;
            }
            if (curfnt->resfont == NULL) {
               cd = curfnt->chardesc + cmd ;
               if ((cd->flags & (PREVPAGE | THISPAGE)) == 0) {
     /* the character hasn't been used before in the current section */
                  cras = cd->packptr ;
                  if (cras != NULL) {
                     thischar = CHARCOST ;
                     if (compressed) {
                        if ((cd->flags & REPACKED) == 0) {
                           repack(cd) ;
                           cras = cd->packptr ;
                        }
                        if ((*cras & 4) == 0)
                           if ((*cras & 7) == 7)
                              thischar += 2 * getlong(cras + 1) * 
                                     ((getlong(cras + 5) + 15) / 16) ;
                           else
                              thischar += 2 * cras[2] *
                                          (long)((cras[1] + 15) / 16) ;
                        else
                           thischar += getlong(cras + 9) ;
                     } else {
                        if ((*cras & 4) == 0)
                           if ((*cras & 7) == 7)
                              thischar += 2 * getlong(cras + 1)
                                          * ((getlong(cras + 5) + 15) / 16) ;
                           else
                              thischar += 2 * cras[2] * ((cras[1] + 15) / 16) ;
                        else
                           thischar += 2 * (cras[3] * 256 + cras[4]) *
                              ((cras[1] * 256 + cras[2] + 15) / 16) ;
                     }
                     cd->flags |= THISPAGE ;
                     fontmem -= thischar ;
                     if (fontmem <= pagecost) {
                        if (pagecount > 0)
                           return(0) ;
                     }
                  }
               }
            }
         }
         break ;        
case 171: case 172: case 173: case 174: case 175: case 176: case 177:
case 178: case 179: case 180: case 181: case 182: case 183: case 184:
case 185: case 186: case 187: case 188: case 189: case 190: case 191:
case 192: case 193: case 194: case 195: case 196: case 197: case 198:
case 199: case 200: case 201: case 202: case 203: case 204: case 205:
case 206: case 207: case 208: case 209: case 210: case 211: case 212:
case 213: case 214: case 215: case 216: case 217: case 218: case 219:
case 220: case 221: case 222: case 223: case 224: case 225: case 226:
case 227: case 228: case 229: case 230: case 231: case 232: case 233:
case 234: case 235: /* font selection commands */
         if (cmd < 235) fnt = cmd - 171 ; /* fntnum0 thru fntnum63 */
         else fnt = dvibyte() ; /* fnt1 */
         instring = 0 ;
         for (cfnt=ffont; cfnt; cfnt = cfnt->next)
            if (cfnt->fontnum == fnt) goto fontfound ;
         error("! Bad DVI file: undefined font selected") ;
fontfound: curfnt = cfnt->desc ;
         if (curfnt->loaded == 0) {
            if (!residentfont(curfnt))
               if (!virtualfont(curfnt))
                  loadfont(curfnt) ;
         }
         if (curfnt->psname == 0) {
            if (curfnt->loaded < 2) /* virtual font costs nothing (yet) */
               fontmem -= (curfnt->resfont ? PSFONTCOST : FONTCOST) ;
            curfnt->psname = 1 ;
            if (fontmem <= pagecost) {
               if (pagecount > 0)
                  return(0) ;
            }
         }
         break ;
case 239: instring = 0 ; predospecial(dvibyte()) ; break ; /* xxx1 */
case 240: instring = 0 ; predospecial(twobytes()) ; break ; /* xxx2 */
case 241: instring = 0 ; predospecial(threebytes()) ; break ; /* xxx3 */
case 242: instring = 0 ; predospecial(signedquad()) ; break ; /* xxx4 */
case 243: fontdef() ; break ; /* fntdef1 */
case 140: /* eop or end of virtual char */
         if (curpos) {
            --frp ;
            curfnt = frp->curf ;
            ffont = frp->ff ;
            curlim = frp->curl ;
            curpos = frp->curp ;
            break ;
         }
         goto endofpage ; /* eop */
      }
   }
endofpage:
   if (fontmem > pagecost)
      return(1) ;
   if (pagecount > 0)
      return(0) ;
#ifdef SHORTINT
      (void)fprintf(stderr, "Page %ld may be too complex to print\n", pagenum) ;
#else   /* ~SHORTINT */
      (void)fprintf(stderr, "Page %d may be too complex to print\n", pagenum) ;
#endif  /* ~SHORTINT */
/*
 *   This case should be rare indeed.  Even with only 200K of virtual memory,
 *   at 11 bytes per char, you can have 18K characters on a page.
 */
   return(2) ;
}
