	.verstamp	1 31
	.extern	screenInfo 64
	.extern	EventSwapVector 0
	.data	
	.align	0
$$1590:
	.ascii	"backing store clip list nil\X00"
	.lcomm	$$839 4
	.sdata	
	.align	2
	.align	0
$$840:
	.word	0 : 1
	.lcomm	$$853 4
	.extern	rootCursor 0
	.data	
	.align	2
	.align	0
$$861:
	.word	$$854
	.word	$$857
	.word	$$855
	.word	$$856
	.word	$$858
	.word	$$859
	.word	$$860
	.byte	0 : 4
	.extern	requestingClient 0
	.data	
	.align	2
	.align	0
$$883:
	.word	$$862
	.word	$$863
	.word	$$864
	.word	$$865
	.word	$$866
	.word	$$867
	.word	$$868
	.word	$$869
	.word	$$870
	.word	$$871
	.word	$$872
	.word	$$873
	.word	$$874
	.word	$$875
	.word	$$876
	.word	$$877
	.word	$$878
	.word	$$879
	.word	$$880
	.word	$$881
	.word	$$882
	.byte	0 : 4
	.extern	clients 0
	.extern	serverClient 0
	.extern	currentMaxClients 4
	.data	
	.align	2
	.align	0
$$891:
	.word	$$884
	.word	$$887
	.word	$$885
	.word	$$886
	.word	$$888
	.word	$$889
	.word	$$890
	.byte	0 : 4
	.data	
	.align	0
$$1419:
	.ascii	"miBSLineHelper called\X0A\X00"
	.extern	globalSerialNumber 4
	.extern	serverGeneration 4
	.extern	DontPropagateMasks 0
	.extern	PixmapWidthPaddingInfo 0
	.extern	currentTime 8
	.text	
	.align	2
	.file	2 "mibstore.c"
	.globl	miInitializeBackingStore
	.loc	2 225
 # 225	{
	.ent	miInitializeBackingStore 2
miInitializeBackingStore:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	sw	$16, 16($sp)
	.mask	0x80010000, -4
	.frame	$sp, 24, $31
	move	$16, $4
	sw	$5, 28($sp)
	.bgnb	895
	.loc	2 228
 # 226	    miBSScreenPtr    pScreenPriv;
 # 227	
 # 228	    if (miBSGeneration != serverGeneration)
	lw	$14, $$840
	lw	$15, serverGeneration
	beq	$14, $15, $32
	.loc	2 230
 # 229	    {
 # 230		miBSScreenIndex = AllocateScreenPrivateIndex ();
	jal	AllocateScreenPrivateIndex
	sw	$2, $$839
	.loc	2 231
 # 231		if (miBSScreenIndex < 0)
	blt	$2, 0, $33
	.loc	2 232
 # 232		    return;
	.loc	2 233
 # 233		miBSGCIndex = AllocateGCPrivateIndex ();
	jal	AllocateGCPrivateIndex
	sw	$2, $$853
	.loc	2 234
 # 234		miBSGeneration = serverGeneration;
	lw	$24, serverGeneration
	sw	$24, $$840
	.loc	2 235
 # 235	    }
$32:
	.loc	2 236
 # 236	    if (!AllocateGCPrivate(pScreen, miBSGCIndex, 0))
	move	$4, $16
	lw	$5, $$853
	move	$6, $0
	jal	AllocateGCPrivate
	beq	$2, 0, $33
	.loc	2 237
 # 237		return;
	.loc	2 238
 # 238	    pScreenPriv = (miBSScreenPtr) xalloc (sizeof (miBSScreenRec));
	li	$4, 28
	jal	Xalloc
	.loc	2 239
 # 239	    if (!pScreenPriv)
	beq	$2, 0, $33
	.loc	2 240
 # 240		return;
	.loc	2 242
 # 241	
 # 242	    pScreenPriv->CloseScreen = pScreen->CloseScreen;
	lw	$25, 128($16)
	sw	$25, 0($2)
	.loc	2 243
 # 243	    pScreenPriv->GetImage = pScreen->GetImage;
	lw	$8, 140($16)
	sw	$8, 4($2)
	.loc	2 244
 # 244	    pScreenPriv->GetSpans = pScreen->GetSpans;
	lw	$9, 144($16)
	sw	$9, 8($2)
	.loc	2 245
 # 245	    pScreenPriv->ChangeWindowAttributes = pScreen->ChangeWindowAttributes;
	lw	$10, 168($16)
	sw	$10, 12($2)
	.loc	2 246
 # 246	    pScreenPriv->CreateGC = pScreen->CreateGC;
	lw	$11, 272($16)
	sw	$11, 16($2)
	.loc	2 247
 # 247	    pScreenPriv->DestroyWindow = pScreen->DestroyWindow;
	lw	$12, 160($16)
	sw	$12, 20($2)
	.loc	2 248
 # 248	    pScreenPriv->funcs = funcs;
	lw	$13, 28($sp)
	sw	$13, 24($2)
	.loc	2 250
 # 249	
 # 250	    pScreen->CloseScreen = miBSCloseScreen;
	la	$14, $$841
	sw	$14, 128($16)
	.loc	2 251
 # 251	    pScreen->GetImage = miBSGetImage;
	la	$15, $$842
	sw	$15, 140($16)
	.loc	2 252
 # 252	    pScreen->GetSpans = miBSGetSpans;
	la	$24, $$843
	sw	$24, 144($16)
	.loc	2 253
 # 253	    pScreen->ChangeWindowAttributes = miBSChangeWindowAttributes;
	la	$25, $$844
	sw	$25, 168($16)
	.loc	2 254
 # 254	    pScreen->CreateGC = miBSCreateGC;
	la	$8, $$845
	sw	$8, 272($16)
	.loc	2 255
 # 255	    pScreen->DestroyWindow = miBSDestroyWindow;
	la	$9, $$846
	sw	$9, 160($16)
	.loc	2 257
 # 256	
 # 257	    pScreen->SaveDoomedAreas = miBSSaveDoomedAreas;
	la	$10, $$847
	sw	$10, 212($16)
	.loc	2 258
 # 258	    pScreen->RestoreAreas = miBSRestoreAreas;
	la	$11, $$848
	sw	$11, 216($16)
	.loc	2 259
 # 259	    pScreen->ExposeCopy = miBSExposeCopy;
	la	$12, $$849
	sw	$12, 220($16)
	.loc	2 260
 # 260	    pScreen->TranslateBackingStore = miBSTranslateBackingStore;
	la	$13, $$850
	sw	$13, 224($16)
	.loc	2 261
 # 261	    pScreen->ClearBackingStore = miBSClearBackingStore;
	la	$14, $$851
	sw	$14, 228($16)
	.loc	2 262
 # 262	    pScreen->DrawGuarantee = miBSDrawGuarantee;
	la	$15, $$852
	sw	$15, 232($16)
	.loc	2 264
 # 263	
 # 264	    pScreen->devPrivates[miBSScreenIndex].ptr = (pointer) pScreenPriv;
	lw	$24, 404($16)
	lw	$25, $$839
	mul	$8, $25, 4
	addu	$9, $24, $8
	sw	$2, 0($9)
	.loc	2 265
 # 265	}
	.endb	898
$33:
	lw	$16, 16($sp)
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miInitializeBackingStore
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 287
 # 287	{
	.ent	$$841 2
$$841:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	sw	$4, 24($sp)
	.bgnb	902
	.loc	2 290
 # 288	    miBSScreenPtr   pScreenPriv;
 # 289	
 # 290	    pScreenPriv = (miBSScreenPtr) pScreen->devPrivates[miBSScreenIndex].ptr;
	lw	$14, 404($5)
	lw	$15, $$839
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$2, 0($25)
	.loc	2 292
 # 291	
 # 292	    pScreen->CloseScreen = pScreenPriv->CloseScreen;
	lw	$8, 0($2)
	sw	$8, 128($5)
	.loc	2 293
 # 293	    pScreen->GetImage = pScreenPriv->GetImage;
	lw	$9, 4($2)
	sw	$9, 140($5)
	.loc	2 294
 # 294	    pScreen->GetSpans = pScreenPriv->GetSpans;
	lw	$10, 8($2)
	sw	$10, 144($5)
	.loc	2 295
 # 295	    pScreen->ChangeWindowAttributes = pScreenPriv->ChangeWindowAttributes;
	lw	$11, 12($2)
	sw	$11, 168($5)
	.loc	2 296
 # 296	    pScreen->CreateGC = pScreenPriv->CreateGC;
	lw	$12, 16($2)
	sw	$12, 272($5)
	.loc	2 298
 # 297	
 # 298	    xfree ((pointer) pScreenPriv);
	move	$4, $2
	sw	$5, 28($sp)
	jal	Xfree
	lw	$5, 28($sp)
	.loc	2 300
 # 299	
 # 300	    return (*pScreen->CloseScreen) (i, pScreen);
	lw	$4, 24($sp)
	lw	$13, 128($5)
	jal	$13
	.endb	904
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCloseScreen
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 312
 # 312	{
	.ent	$$842 2
$$842:
	.option	O2
	subu	$sp, 208
	sw	$31, 44($sp)
	sw	$23, 40($sp)
	.mask	0x80800000, -164
	.frame	$sp, 208, $31
	move	$23, $4
	sw	$5, 212($sp)
	sw	$6, 216($sp)
	sw	$7, 220($sp)
	.bgnb	916
	.loc	2 313
 # 313	    ScreenPtr		    pScreen = pDrawable->pScreen;
	lw	$2, 16($23)
	.loc	2 317
 # 314	    BoxRec		    bounds;
 # 315	    unsigned char	    depth;
 # 316	    
 # 317	    SCREEN_PROLOGUE (pScreen, GetImage);
	lw	$14, 404($2)
	lw	$15, $$839
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	lw	$9, 4($8)
	sw	$9, 140($2)
	.loc	2 319
 # 318	
 # 319	    if (pDrawable->type == DRAWABLE_WINDOW)
	lbu	$10, 0($23)
	bne	$10, 0, $64
	sw	$18, 80($sp)
	sw	$30, 76($sp)
	.bgnb	920
	.loc	2 332
 # 332		pWin = (WindowPtr) pDrawable;
	move	$18, $23
	.loc	2 333
 # 333		pPixmap = 0;
	move	$30, $0
	.loc	2 334
 # 334		depth = pDrawable->depth;
	lbu	$11, 2($23)
	sb	$11, 195($sp)
	.loc	2 335
 # 335		bounds.x1 = sx + pDrawable->x;
	lw	$12, 212($sp)
	lh	$13, 8($23)
	addu	$15, $12, $13
	sh	$15, 196($sp)
	.loc	2 336
 # 336		bounds.y1 = sy + pDrawable->y;
	lw	$14, 216($sp)
	lh	$24, 10($23)
	addu	$25, $14, $24
	sh	$25, 198($sp)
	.loc	2 337
 # 337		bounds.x2 = bounds.x1 + w;
	lw	$8, 220($sp)
	sll	$9, $15, 16
	sra	$10, $9, 16
	addu	$11, $10, $8
	sh	$11, 200($sp)
	.loc	2 338
 # 338		bounds.y2 = bounds.y1 + h;
	lw	$12, 224($sp)
	sll	$13, $25, 16
	sra	$14, $13, 16
	addu	$24, $14, $12
	sh	$24, 202($sp)
	.loc	2 339
 # 339		(*pScreen->RegionInit) (&Remaining, &bounds, 0);
	addu	$4, $sp, 152
	addu	$5, $sp, 196
	move	$6, $0
	sw	$2, 204($sp)
	lw	$15, 308($2)
	jal	$15
	sw	$16, 72($sp)
	sw	$17, 68($sp)
	sw	$19, 64($sp)
	sw	$20, 60($sp)
	sw	$21, 56($sp)
	sw	$22, 52($sp)
	lw	$3, 224($sp)
	lw	$7, 220($sp)
	lw	$22, 180($sp)
$34:
	.loc	2 342
 # 340		for (;;)
 # 341	 	{
 # 342		    bounds.x1 = sx + pDrawable->x - pWin->drawable.x;
	lw	$9, 212($sp)
	lh	$10, 8($23)
	addu	$8, $9, $10
	lh	$11, 8($18)
	subu	$25, $8, $11
	sh	$25, 196($sp)
	.loc	2 343
 # 343		    bounds.y1 = sy + pDrawable->y - pWin->drawable.y;
	lw	$13, 216($sp)
	lh	$14, 10($23)
	addu	$12, $13, $14
	lh	$24, 10($18)
	subu	$15, $12, $24
	sh	$15, 198($sp)
	.loc	2 344
 # 344		    bounds.x2 = bounds.x1 + w;
	sll	$9, $25, 16
	sra	$10, $9, 16
	addu	$8, $10, $7
	sh	$8, 200($sp)
	.loc	2 345
 # 345		    bounds.y2 = bounds.y1 + h;
	sll	$11, $15, 16
	sra	$13, $11, 16
	addu	$14, $13, $3
	sh	$14, 202($sp)
	.loc	2 352
 # 352			  (*pScreen->RegionExtents) (&pWin->borderSize)) != rgnOUT))
	lw	$12, 124($18)
	sll	$24, $12, 10
	srl	$25, $24, 31
	beq	$25, $0, $57
	lw	$2, 116($18)
	beq	$2, $0, $57
	lbu	$9, 2($18)
	lbu	$10, 195($sp)
	bne	$9, $10, $57
	move	$21, $2
	addu	$8, $21, 4
	sw	$8, 88($sp)
	move	$4, $8
	addu	$5, $sp, 196
	lw	$15, 204($sp)
	lw	$11, 348($15)
	jal	$11
	bne	$2, 0, $35
	addu	$4, $18, 84
	lw	$13, 204($sp)
	lw	$14, 364($13)
	jal	$14
	addu	$4, $sp, 152
	move	$5, $2
	lw	$12, 204($sp)
	lw	$24, 348($12)
	jal	$24
	bne	$2, 0, $35
	lw	$7, 220($sp)
	lw	$3, 224($sp)
	b	$57
$35:
	.loc	2 354
 # 353		    {
 # 354			if (!pPixmap)
	bne	$30, 0, $39
	lw	$4, 204($sp)
	.bgnb	933
	.loc	2 356
 # 355			{
 # 356			    XID	subWindowMode = IncludeInferiors;
	li	$25, 1
	sw	$25, 104($sp)
	.loc	2 359
 # 357			    int	x, y;
 # 358	
 # 359			    pPixmap = (*pScreen->CreatePixmap) (pScreen, w, h, depth);
	lw	$5, 220($sp)
	lw	$6, 224($sp)
	lbu	$7, 195($sp)
	lw	$9, 204($4)
	jal	$9
	move	$30, $2
	.loc	2 360
 # 360			    if (!pPixmap)
	bne	$2, 0, $36
	.loc	2 361
 # 361				goto punt;
	lw	$16, 72($sp)
	lw	$17, 68($sp)
	lw	$18, 80($sp)
	lw	$19, 64($sp)
	lw	$20, 60($sp)
	lw	$21, 56($sp)
	lw	$22, 52($sp)
	lw	$30, 76($sp)
	lw	$3, 224($sp)
	b	$65
$36:
	.loc	2 362
 # 362			    pGC = GetScratchGC (depth, pScreen);
	lbu	$4, 195($sp)
	lw	$5, 204($sp)
	jal	GetScratchGC
	move	$22, $2
	.loc	2 363
 # 363			    if (!pGC)
	bne	$2, 0, $37
	.loc	2 365
 # 364			    {
 # 365				(*pScreen->DestroyPixmap) (pPixmap);
	move	$4, $30
	lw	$10, 204($sp)
	lw	$8, 208($10)
	jal	$8
	lw	$16, 72($sp)
	lw	$17, 68($sp)
	lw	$18, 80($sp)
	lw	$19, 64($sp)
	lw	$20, 60($sp)
	lw	$21, 56($sp)
	lw	$22, 52($sp)
	lw	$30, 76($sp)
	.loc	2 366
 # 366				goto punt;
	lw	$3, 224($sp)
	b	$65
$37:
	.loc	2 368
 # 367			    }
 # 368			    ChangeGC (pGC, GCSubwindowMode, &subWindowMode);
	move	$4, $22
	li	$5, 32768
	addu	$6, $sp, 104
	move	$16, $23
	lw	$17, 212($sp)
	lw	$19, 216($sp)
	addu	$20, $23, 56
	jal	ChangeGC
	.loc	2 369
 # 369			    ValidateGC ((DrawablePtr)pPixmap, pGC);
	move	$4, $30
	move	$5, $22
	jal	ValidateGC
	.loc	2 370
 # 370			    (*pScreen->RegionInit) (&Border, NullBox, 0);
	addu	$4, $sp, 140
	move	$5, $0
	move	$6, $0
	lw	$15, 204($sp)
	lw	$11, 308($15)
	jal	$11
	.loc	2 371
 # 371			    (*pScreen->RegionInit) (&Inside, NullBox, 0);
	addu	$4, $sp, 128
	move	$5, $0
	move	$6, $0
	lw	$13, 204($sp)
	lw	$14, 308($13)
	jal	$14
	.loc	2 372
 # 372			    pSrcWin = (WindowPtr) pDrawable;
	.loc	2 373
 # 373			    x = sx;
	.loc	2 374
 # 374			    y = sy;
	.loc	2 375
 # 375			    if (pSrcWin->parent)
	lw	$2, 24($23)
	beq	$2, $0, $38
	.loc	2 377
 # 376			    {
 # 377				x += pSrcWin->origin.x;
	lw	$12, 212($sp)
	lh	$24, 96($23)
	addu	$17, $12, $24
	.loc	2 378
 # 378				y += pSrcWin->origin.y;
	lw	$25, 216($sp)
	lh	$9, 98($23)
	addu	$19, $25, $9
	.loc	2 379
 # 379				pSrcWin = pSrcWin->parent;
	move	$16, $2
	.loc	2 380
 # 380			    }
$38:
	.loc	2 384
 # 381			    (*pGC->ops->CopyArea) (pSrcWin,
 # 382	 					    pPixmap, pGC,
 # 383						    x, y, w, h,
 # 384						    0, 0);
	move	$4, $16
	move	$5, $30
	move	$6, $22
	move	$7, $17
	sw	$19, 16($sp)
	lw	$10, 220($sp)
	sw	$10, 20($sp)
	lw	$8, 224($sp)
	sw	$8, 24($sp)
	sw	$0, 28($sp)
	sw	$0, 32($sp)
	lw	$15, 72($22)
	lw	$11, 12($15)
	jal	$11
	.loc	2 386
 # 385			    (*pScreen->Subtract) (&Remaining, &Remaining,
 # 386					          &((WindowPtr) pDrawable)->borderClip);
	addu	$13, $sp, 152
	move	$4, $13
	move	$5, $13
	move	$6, $20
	lw	$14, 204($sp)
	lw	$12, 332($14)
	jal	$12
	.loc	2 387
 # 387			}
	.endb	937
$39:
	.loc	2 389
 # 388	
 # 389			(*pScreen->Intersect) (&Inside, &Remaining, &pWin->winSize);
	addu	$4, $sp, 128
	addu	$5, $sp, 152
	addu	$24, $18, 72
	sw	$24, 84($sp)
	move	$6, $24
	addu	$25, $18, 84
	sw	$25, 92($sp)
	lw	$9, 204($sp)
	lw	$10, 324($9)
	jal	$10
	.loc	2 392
 # 390			(*pScreen->TranslateRegion) (&Inside,
 # 391						     -pWin->drawable.x,
 # 392	 					     -pWin->drawable.y);
	addu	$4, $sp, 128
	lh	$5, 8($18)
	negu	$5, $5
	lh	$6, 10($18)
	negu	$6, $6
	lw	$8, 204($sp)
	lw	$15, 344($8)
	jal	$15
	.loc	2 393
 # 393			(*pScreen->Intersect) (&Inside, &Inside, &pWindowPriv->SavedRegion);
	addu	$11, $sp, 128
	move	$4, $11
	move	$5, $11
	lw	$6, 88($sp)
	lw	$13, 204($sp)
	lw	$14, 324($13)
	jal	$14
	.loc	2 395
 # 394	
 # 395			xoff = pWin->drawable.x - pDrawable->x - sx;
	lh	$12, 8($18)
	lh	$24, 8($23)
	subu	$25, $12, $24
	lw	$9, 212($sp)
	subu	$19, $25, $9
	.loc	2 396
 # 396			yoff = pWin->drawable.y - pDrawable->y - sy;
	lh	$10, 10($18)
	lh	$8, 10($23)
	subu	$15, $10, $8
	lw	$11, 216($sp)
	subu	$20, $15, $11
	.loc	2 398
 # 397	
 # 398			if (REGION_NUM_RECTS(&Inside) > 0)
	lw	$13, 136($sp)
	beq	$13, 0, $40
	lw	$2, 4($13)
	b	$41
$40:
	li	$2, 1
$41:
	ble	$2, 0, $51
	.loc	2 400
 # 399			{
 # 400			    switch (pWindowPriv->status)
	lb	$2, 17($21)
	b	$50
$42:
	lw	$2, 136($sp)
	.loc	2 403
 # 401			    {
 # 402			    case StatusContents:
 # 403				pBox = REGION_RECTS(&Inside);
	beq	$2, 0, $43
	addu	$16, $2, 8
	b	$44
$43:
	addu	$16, $sp, 128
$44:
	.loc	2 404
 # 404				for (n = REGION_NUM_RECTS(&Inside); --n >= 0;)
	beq	$2, 0, $45
	lw	$14, 136($sp)
	lw	$17, 4($14)
	b	$46
$45:
	li	$17, 1
$46:
	.loc	2 404
	addu	$17, $17, -1
	blt	$17, 0, $51
$47:
	.loc	2 412
 # 412							   pBox->y1 + yoff);
	lw	$4, 0($21)
	move	$5, $30
	move	$6, $22
	lh	$7, 0($16)
	lh	$2, 2($16)
	sw	$2, 16($sp)
	lh	$12, 4($16)
	subu	$24, $12, $7
	sw	$24, 20($sp)
	lh	$25, 6($16)
	subu	$9, $25, $2
	sw	$9, 24($sp)
	addu	$10, $7, $19
	sw	$10, 28($sp)
	addu	$8, $2, $20
	sw	$8, 32($sp)
	lw	$15, 72($22)
	lw	$11, 12($15)
	jal	$11
	.loc	2 413
 # 413				    ++pBox;
	addu	$16, $16, 8
	.loc	2 414
 # 414				}
	.loc	2 414
	addu	$17, $17, -1
	bge	$17, 0, $47
	.loc	2 415
 # 415				break;
	b	$51
$48:
	.loc	2 419
 # 416			    case StatusVirtual:
 # 417			    case StatusVDirty:
 # 418				if (pWindowPriv->backgroundState == BackgroundPixmap ||
 # 419				    pWindowPriv->backgroundState == BackgroundPixel)
	lb	$2, 18($21)
	beq	$2, 3, $49
	bne	$2, 2, $51
$49:
	.loc	2 423
 # 420				miBSFillVirtualBits ((DrawablePtr) pPixmap, pGC, &Inside,
 # 421						    -xoff, -yoff,
 # 422						    pWindowPriv->backgroundState,
 # 423						    pWindowPriv->background, ~0L);
	move	$4, $30
	move	$5, $22
	addu	$6, $sp, 128
	negu	$7, $19
	negu	$13, $20
	sw	$13, 16($sp)
	sw	$2, 20($sp)
	.set	 noat
	lw	$1, 20($21)
	sw	$1, 24($sp)
	.set	 at
	li	$12, -1
	sw	$12, 28($sp)
	jal	$$907
	.loc	2 424
 # 424				break;
	b	$51
$50:
	beq	$2, 2, $48
	beq	$2, 3, $48
	beq	$2, 5, $42
	.loc	2 426
 # 425			    }
 # 426			}
$51:
	.loc	2 427
 # 427			(*pScreen->Subtract) (&Border, &pWin->borderSize, &pWin->winSize);
	addu	$4, $sp, 140
	lw	$5, 92($sp)
	lw	$6, 84($sp)
	lw	$24, 204($sp)
	lw	$25, 332($24)
	jal	$25
	.loc	2 428
 # 428			(*pScreen->Intersect) (&Border, &Border, &Remaining);
	addu	$9, $sp, 140
	move	$4, $9
	move	$5, $9
	addu	$6, $sp, 152
	lw	$10, 204($sp)
	lw	$8, 324($10)
	jal	$8
	.loc	2 429
 # 429			if (REGION_NUM_RECTS(&Border) > 0)
	lw	$15, 148($sp)
	beq	$15, 0, $52
	lw	$2, 4($15)
	b	$53
$52:
	li	$2, 1
$53:
	ble	$2, 0, $56
	.loc	2 432
 # 430			{
 # 431			    (*pScreen->TranslateRegion)  (&Border, -pWin->drawable.x,
 # 432							  -pWin->drawable.y);
	addu	$4, $sp, 140
	lh	$5, 8($18)
	negu	$5, $5
	lh	$6, 10($18)
	negu	$6, $6
	negu	$16, $19
	negu	$17, $20
	lw	$11, 204($sp)
	lw	$13, 344($11)
	jal	$13
	.loc	2 436
 # 433			    miBSFillVirtualBits ((DrawablePtr) pPixmap, pGC, &Border,
 # 434					    	-xoff, -yoff,
 # 435					    	pWin->borderIsPixel ? (int)BackgroundPixel : (int)BackgroundPixmap,
 # 436					    	pWin->border, ~0L);
	lw	$14, 124($18)
	sll	$12, $14, 29
	srl	$24, $12, 31
	beq	$24, $0, $54
	li	$2, 2
	b	$55
$54:
	li	$2, 3
$55:
	move	$4, $30
	move	$5, $22
	addu	$6, $sp, 140
	move	$7, $16
	sw	$17, 16($sp)
	sw	$2, 20($sp)
	.set	 noat
	lw	$1, 112($18)
	sw	$1, 24($sp)
	.set	 at
	li	$9, -1
	sw	$9, 28($sp)
	jal	$$907
	.loc	2 437
 # 437			}
$56:
	.loc	2 438
 # 438		    }
	lw	$3, 224($sp)
	lw	$7, 220($sp)
$57:
	.loc	2 440
 # 439	
 # 440		    if (pWin->viewable && pWin->firstChild)
	lw	$10, 124($18)
	sll	$8, $10, 10
	srl	$15, $8, 31
	beq	$15, $0, $58
	lw	$2, 36($18)
	beq	$2, $0, $58
	.loc	2 441
 # 441			pWin = pWin->firstChild;
	move	$18, $2
	b	$34
$58:
	.loc	2 444
 # 442		    else
 # 443		    {
 # 444			while (!pWin->nextSib && pWin != (WindowPtr) pDrawable)
	lw	$2, 28($18)
	bne	$2, $0, $60
	beq	$18, $23, $60
$59:
	.loc	2 445
 # 445			    pWin = pWin->parent;
	lw	$18, 24($18)
	.loc	2 445
	lw	$2, 28($18)
	bne	$2, $0, $60
	bne	$18, $23, $59
$60:
	.loc	2 446
 # 446			if (pWin == (WindowPtr) pDrawable)
	beq	$18, $23, $61
	.loc	2 447
 # 447			    break;
	.loc	2 448
 # 448			pWin = pWin->nextSib;
	move	$18, $2
	.loc	2 449
 # 449		    }
	.loc	2 450
 # 450		}
	b	$34
$61:
	.loc	2 452
 # 451	
 # 452		if (pPixmap)
	beq	$30, 0, $62
	.loc	2 455
 # 453		{
 # 454		    (*pScreen->GetImage) ((DrawablePtr) pPixmap,
 # 455			0, 0, w, h, format, planemask, pdstLine);
	move	$4, $30
	move	$5, $0
	move	$6, $0
	sw	$3, 16($sp)
	lw	$11, 228($sp)
	sw	$11, 20($sp)
	lw	$13, 232($sp)
	sw	$13, 24($sp)
	lw	$14, 236($sp)
	sw	$14, 28($sp)
	lw	$12, 204($sp)
	lw	$24, 140($12)
	jal	$24
	.loc	2 456
 # 456		    (*pScreen->DestroyPixmap) (pPixmap);
	move	$4, $30
	lw	$25, 204($sp)
	lw	$9, 208($25)
	jal	$9
	.loc	2 457
 # 457		    FreeScratchGC (pGC);
	move	$4, $22
	jal	FreeScratchGC
	.loc	2 458
 # 458		}
	b	$63
$62:
	lw	$16, 72($sp)
	lw	$17, 68($sp)
	lw	$18, 80($sp)
	lw	$19, 64($sp)
	lw	$20, 60($sp)
	lw	$21, 56($sp)
	.loc	2 461
 # 459		else
 # 460		{
 # 461		    goto punt;
	lw	$22, 52($sp)
	lw	$30, 76($sp)
	b	$65
$63:
	lw	$16, 72($sp)
	lw	$17, 68($sp)
	lw	$18, 80($sp)
	lw	$19, 64($sp)
	lw	$20, 60($sp)
	lw	$21, 56($sp)
	lw	$22, 52($sp)
	lw	$30, 76($sp)
	.loc	2 463
 # 462		}
 # 463	    }
	.endb	938
	b	$66
$64:
	sw	$2, 204($sp)
	lw	$3, 224($sp)
$65:
	.lab	punt
	lw	$2, 204($sp)
	.loc	2 468
 # 464	    else
 # 465	    {
 # 466	punt:	;
 # 467		(*pScreen->GetImage) (pDrawable, sx, sy, w, h,
 # 468				      format, planemask, pdstLine);
	move	$4, $23
	lw	$5, 212($sp)
	lw	$6, 216($sp)
	lw	$7, 220($sp)
	sw	$3, 16($sp)
	lw	$10, 228($sp)
	sw	$10, 20($sp)
	lw	$8, 232($sp)
	sw	$8, 24($sp)
	lw	$15, 236($sp)
	sw	$15, 28($sp)
	lw	$11, 140($2)
	jal	$11
	.loc	2 469
 # 469	    }
$66:
	.loc	2 471
 # 470	
 # 471	    SCREEN_EPILOGUE (pScreen, GetImage, miBSGetImage);
	la	$13, $$842
	lw	$14, 204($sp)
	sw	$13, 140($14)
	.loc	2 472
 # 472	}
	.endb	940
	lw	$23, 40($sp)
	lw	$31, 44($sp)
	addu	$sp, 208
	j	$31
	.end	miBSGetImage
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 482
 # 482	{
	.ent	$$843 2
$$843:
	.option	O2
	subu	$sp, 112
	sw	$31, 44($sp)
	sw	$16, 40($sp)
	.mask	0x80010000, -68
	.frame	$sp, 112, $31
	move	$10, $4
	sw	$5, 116($sp)
	move	$9, $6
	move	$12, $7
	.bgnb	949
	.loc	2 483
 # 483	    ScreenPtr		    pScreen = pDrawable->pScreen;
	lw	$11, 16($10)
	.loc	2 489
 # 484	    BoxRec		    bounds;
 # 485	    int			    i;
 # 486	    WindowPtr		    pWin;
 # 487	    int			    dx, dy;
 # 488	    
 # 489	    SCREEN_PROLOGUE (pScreen, GetSpans);
	lw	$14, 404($11)
	lw	$15, $$839
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$13, 0($25)
	lw	$15, 8($13)
	sw	$15, 144($11)
	.loc	2 491
 # 490	
 # 491	    if (pDrawable->type == DRAWABLE_WINDOW && ((WindowPtr) pDrawable)->backStorage)
	lbu	$14, 0($10)
	bne	$14, 0, $82
	lw	$2, 116($10)
	beq	$2, $0, $82
	lw	$16, 128($sp)
	.bgnb	956
	.loc	2 497
 # 492	    {
 # 493		PixmapPtr	pPixmap;
 # 494		miBSWindowPtr	pWindowPriv;
 # 495		GCPtr		pGC;
 # 496	
 # 497		pWin = (WindowPtr) pDrawable;
	.loc	2 498
 # 498		pWindowPriv = (miBSWindowPtr) pWin->backStorage;
	move	$7, $2
	.loc	2 499
 # 499		pPixmap = pWindowPriv->pBackingPixmap;
	lw	$8, 0($7)
	.loc	2 501
 # 500	
 # 501	    	bounds.x1 = ppt->x;
	lh	$24, 0($9)
	sh	$24, 100($sp)
	.loc	2 502
 # 502	    	bounds.y1 = ppt->y;
	lh	$25, 2($9)
	sh	$25, 102($sp)
	.loc	2 503
 # 503	    	bounds.x2 = bounds.x1 + *pwidth;
	lh	$13, 100($sp)
	lw	$15, 0($12)
	addu	$14, $13, $15
	sh	$14, 104($sp)
	.loc	2 504
 # 504	    	bounds.y2 = ppt->y;
	lh	$24, 2($9)
	sh	$24, 106($sp)
	.loc	2 505
 # 505	    	for (i = 0; i < nspans; i++)
	move	$5, $0
	.loc	2 505
	ble	$16, 0, $72
	move	$4, $9
	move	$6, $12
$67:
	.loc	2 507
 # 506	    	{
 # 507		    if (ppt[i].x < bounds.x1)
	lh	$2, 0($4)
	lh	$25, 100($sp)
	bge	$2, $25, $68
	.loc	2 508
 # 508		    	bounds.x1 = ppt[i].x;
	sh	$2, 100($sp)
	lh	$2, 0($4)
$68:
	.loc	2 509
 # 509		    if (ppt[i].x + pwidth[i] > bounds.x2)
	lw	$13, 0($6)
	addu	$3, $2, $13
	lh	$15, 104($sp)
	ble	$3, $15, $69
	.loc	2 510
 # 510		    	bounds.x2 = ppt[i].x + pwidth[i];
	sh	$3, 104($sp)
$69:
	.loc	2 511
 # 511		    if (ppt[i].y < bounds.y1)
	lh	$2, 2($4)
	lh	$14, 102($sp)
	bge	$2, $14, $70
	.loc	2 512
 # 512		    	bounds.y1 = ppt[i].y;
	sh	$2, 102($sp)
	b	$71
$70:
	.loc	2 513
 # 513		    else if (ppt[i].y > bounds.y2)
	lh	$24, 106($sp)
	ble	$2, $24, $71
	.loc	2 514
 # 514		    	bounds.y2 = ppt[i].y;
	sh	$2, 106($sp)
$71:
	.loc	2 515
 # 515	    	}
	.loc	2 515
	addu	$5, $5, 1
	addu	$4, $4, 4
	addu	$6, $6, 4
	.loc	2 515
	blt	$5, $16, $67
$72:
	.loc	2 517
 # 516	    
 # 517	    	switch ((*pScreen->RectIn) (&pWindowPriv->SavedRegion, &bounds))
	addu	$4, $7, 4
	addu	$5, $sp, 100
	sw	$7, 76($sp)
	sw	$8, 80($sp)
	sw	$9, 120($sp)
	sw	$10, 112($sp)
	sw	$11, 108($sp)
	sw	$12, 124($sp)
	lw	$25, 348($11)
	jal	$25
	lw	$8, 80($sp)
	b	$81
$73:
	.loc	2 520
 # 518	 	{
 # 519		case rgnPART:
 # 520		    if (!pPixmap)
	bne	$8, 0, $74
	.loc	2 522
 # 521		    {
 # 522			miCreateBSPixmap (pWin);
	lw	$4, 112($sp)
	jal	$$833
	.loc	2 523
 # 523			if (!(pPixmap = pWindowPriv->pBackingPixmap))
	lw	$13, 76($sp)
	lw	$8, 0($13)
	beq	$8, 0, $83
	.loc	2 524
 # 524			    break;
	.loc	2 525
 # 525		    }
$74:
	.loc	2 526
 # 526		    pWindowPriv->status = StatusNoPixmap;
	li	$15, 1
	lw	$14, 76($sp)
	sb	$15, 17($14)
	.loc	2 528
 # 527		    pGC = GetScratchGC(pPixmap->drawable.depth,
 # 528				       pPixmap->drawable.pScreen);
	lbu	$4, 2($8)
	lw	$5, 16($8)
	sw	$8, 80($sp)
	jal	GetScratchGC
	lw	$8, 80($sp)
	move	$6, $2
	.loc	2 529
 # 529		    if (pGC)
	beq	$2, 0, $75
	.loc	2 531
 # 530		    {
 # 531			ValidateGC ((DrawablePtr) pPixmap, pGC);
	move	$4, $8
	move	$5, $6
	sw	$6, 72($sp)
	sw	$8, 80($sp)
	jal	ValidateGC
	lw	$4, 112($sp)
	lw	$6, 72($sp)
	lw	$8, 80($sp)
	.loc	2 537
 # 532			(*pGC->ops->CopyArea)
 # 533			    (pDrawable, (DrawablePtr) pPixmap, pGC,
 # 534			    bounds.x1, bounds.y1,
 # 535			    bounds.x2 - bounds.x1, bounds.y2 - bounds.y1,
 # 536			    bounds.x1 + pPixmap->drawable.x - pWin->drawable.x,
 # 537			    bounds.y1 + pPixmap->drawable.y - pWin->drawable.y);
	move	$5, $8
	lh	$24, 100($sp)
	move	$7, $24
	lh	$25, 102($sp)
	sw	$25, 16($sp)
	lh	$13, 104($sp)
	subu	$15, $13, $24
	sw	$15, 20($sp)
	lh	$14, 106($sp)
	subu	$13, $14, $25
	sw	$13, 24($sp)
	lh	$15, 8($8)
	addu	$14, $24, $15
	lh	$13, 8($4)
	subu	$24, $14, $13
	sw	$24, 28($sp)
	lh	$15, 10($8)
	addu	$14, $25, $15
	lh	$13, 10($4)
	subu	$24, $14, $13
	sw	$24, 32($sp)
	lw	$25, 72($6)
	lw	$15, 12($25)
	jal	$15
	lw	$6, 72($sp)
	lw	$8, 80($sp)
	.loc	2 538
 # 538			FreeScratchGC(pGC);
	move	$4, $6
	jal	FreeScratchGC
	lw	$8, 80($sp)
	.loc	2 539
 # 539		    }
$75:
	.loc	2 540
 # 540		    pWindowPriv->status = StatusContents;
	li	$14, 5
	lw	$13, 76($sp)
	sb	$14, 17($13)
$76:
	.loc	2 543
 # 541		    /* fall through */
 # 542		case rgnIN:
 # 543		    if (!pPixmap)
	bne	$8, 0, $77
	.loc	2 545
 # 544		    {
 # 545			miCreateBSPixmap (pWin);
	lw	$4, 112($sp)
	jal	$$833
	.loc	2 546
 # 546			if (!(pPixmap = pWindowPriv->pBackingPixmap))
	lw	$24, 76($sp)
	lw	$8, 0($24)
	beq	$8, 0, $83
	.loc	2 547
 # 547			    break;
	.loc	2 548
 # 548		    }
$77:
	.loc	2 549
 # 549		    dx = pPixmap->drawable.x - pWin->drawable.x;
	lh	$25, 8($8)
	lw	$15, 112($sp)
	lh	$14, 8($15)
	subu	$2, $25, $14
	.loc	2 550
 # 550		    dy = pPixmap->drawable.y - pWin->drawable.y;
	lh	$13, 10($8)
	lh	$24, 10($15)
	subu	$3, $13, $24
	.loc	2 551
 # 551		    for (i = 0; i < nspans; i++)
	move	$5, $0
	.loc	2 551
	ble	$16, 0, $79
	lw	$4, 120($sp)
$78:
	.loc	2 553
 # 552		    {
 # 553			ppt[i].x += dx;
	lh	$25, 0($4)
	addu	$14, $25, $2
	sh	$14, 0($4)
	.loc	2 554
 # 554			ppt[i].y += dy;
	lh	$15, 2($4)
	addu	$13, $15, $3
	sh	$13, 2($4)
	.loc	2 555
 # 555		    }
	.loc	2 555
	addu	$5, $5, 1
	addu	$4, $4, 4
	.loc	2 555
	blt	$5, $16, $78
$79:
	.loc	2 557
 # 556		    (*pScreen->GetSpans) ((DrawablePtr) pPixmap,
 # 557					  wMax, ppt, pwidth, nspans, pdstStart);
	move	$4, $8
	lw	$5, 116($sp)
	lw	$6, 120($sp)
	lw	$7, 124($sp)
	sw	$16, 16($sp)
	lw	$24, 132($sp)
	sw	$24, 20($sp)
	lw	$25, 108($sp)
	lw	$14, 144($25)
	jal	$14
	.loc	2 558
 # 558		    break;
	b	$83
$80:
	.loc	2 561
 # 559		case rgnOUT:
 # 560		    (*pScreen->GetSpans) (pDrawable, wMax, ppt, pwidth, nspans,
 # 561					  pdstStart);
	lw	$4, 112($sp)
	lw	$5, 116($sp)
	lw	$6, 120($sp)
	lw	$7, 124($sp)
	sw	$16, 16($sp)
	lw	$15, 132($sp)
	sw	$15, 20($sp)
	lw	$13, 108($sp)
	lw	$24, 144($13)
	jal	$24
	.loc	2 562
 # 562		    break;
	b	$83
$81:
	beq	$2, 0, $80
	beq	$2, 1, $76
	beq	$2, 2, $73
	.loc	2 564
 # 563		}
 # 564	    }
	.endb	960
	b	$83
$82:
	lw	$16, 128($sp)
	.loc	2 567
 # 565	    else
 # 566	    {
 # 567		(*pScreen->GetSpans) (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
	move	$4, $10
	lw	$5, 116($sp)
	move	$6, $9
	move	$7, $12
	sw	$16, 16($sp)
	lw	$25, 132($sp)
	sw	$25, 20($sp)
	sw	$11, 108($sp)
	lw	$14, 144($11)
	jal	$14
	.loc	2 568
 # 568	    }
$83:
	.loc	2 570
 # 569	
 # 570	    SCREEN_EPILOGUE (pScreen, GetSpans, miBSGetSpans);
	la	$15, $$843
	lw	$13, 108($sp)
	sw	$15, 144($13)
	.loc	2 571
 # 571	}
	.endb	961
	lw	$16, 40($sp)
	lw	$31, 44($sp)
	addu	$sp, 112
	j	$31
	.end	miBSGetSpans
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 577
 # 572	
 # 573	static Bool
 # 574	miBSChangeWindowAttributes (pWin, mask)
 # 575	    WindowPtr	    pWin;
 # 576	    unsigned long   mask;
 # 577	{
	.ent	$$844 2
$$844:
	.option	O2
	subu	$sp, 32
	sw	$31, 20($sp)
	sw	$16, 16($sp)
	.mask	0x80010000, -12
	.frame	$sp, 32, $31
	sw	$5, 36($sp)
	.bgnb	966
	.loc	2 581
 # 578	    ScreenPtr	pScreen;
 # 579	    Bool	ret;
 # 580	
 # 581	    pScreen = pWin->drawable.pScreen;
	lw	$16, 16($4)
	.loc	2 583
 # 582	
 # 583	    SCREEN_PROLOGUE (pScreen, ChangeWindowAttributes);
	lw	$14, 404($16)
	lw	$15, $$839
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	lw	$9, 12($8)
	sw	$9, 168($16)
	.loc	2 585
 # 584	
 # 585	    ret = (*pScreen->ChangeWindowAttributes) (pWin, mask);
	lw	$5, 36($sp)
	sw	$4, 32($sp)
	lw	$10, 168($16)
	jal	$10
	lw	$4, 32($sp)
	sw	$2, 24($sp)
	.loc	2 587
 # 586	
 # 587	    if (ret && (mask & CWBackingStore))
	beq	$2, 0, $86
	lw	$11, 36($sp)
	and	$12, $11, 64
	beq	$12, $0, $86
	.loc	2 589
 # 588	    {
 # 589		if (pWin->backingStore != NotUseful || pWin->DIXsaveUnder)
	lw	$2, 124($4)
	sll	$13, $2, 26
	srl	$15, $13, 30
	bne	$15, 0, $84
	sll	$14, $2, 24
	srl	$24, $14, 31
	beq	$24, $0, $85
$84:
	.loc	2 590
 # 590		    miBSAllocate (pWin);
	jal	$$836
	b	$86
$85:
	.loc	2 592
 # 591		else
 # 592		    miBSFree (pWin);
	jal	$$837
	.loc	2 593
 # 593	    }
$86:
	.loc	2 595
 # 594	
 # 595	    SCREEN_EPILOGUE (pScreen, ChangeWindowAttributes, miBSChangeWindowAttributes);
	la	$25, $$844
	sw	$25, 168($16)
	.loc	2 597
 # 596	
 # 597	    return ret;
	lw	$2, 24($sp)
	.endb	969
	lw	$16, 16($sp)
	lw	$31, 20($sp)
	addu	$sp, 32
	j	$31
	.end	miBSChangeWindowAttributes
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 608
 # 608	{
	.ent	$$845 2
$$845:
	.option	O2
	subu	$sp, 32
	sw	$31, 20($sp)
	.mask	0x80000000, -12
	.frame	$sp, 32, $31
	.bgnb	973
	.loc	2 609
 # 609	    ScreenPtr	pScreen = pGC->pScreen;
	lw	$3, 0($4)
	.loc	2 612
 # 610	    Bool	ret;
 # 611	
 # 612	    SCREEN_PROLOGUE (pScreen, CreateGC);
	lw	$14, 404($3)
	lw	$15, $$839
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	lw	$9, 16($8)
	sw	$9, 272($3)
	.loc	2 614
 # 613	    
 # 614	    if (ret = (*pScreen->CreateGC) (pGC))
	sw	$3, 28($sp)
	sw	$4, 32($sp)
	lw	$10, 272($3)
	jal	$10
	lw	$3, 28($sp)
	lw	$4, 32($sp)
	move	$5, $2
	beq	$2, 0, $87
	.loc	2 616
 # 615	    {
 # 616	    	pGC->devPrivates[miBSGCIndex].ptr = (pointer) pGC->funcs;
	lw	$11, 68($4)
	lw	$12, 76($4)
	lw	$13, $$853
	mul	$15, $13, 4
	addu	$14, $12, $15
	sw	$11, 0($14)
	.loc	2 617
 # 617	    	pGC->funcs = &miBSCheapGCFuncs;
	la	$24, $$891
	sw	$24, 68($4)
	.loc	2 618
 # 618	    }
$87:
	.loc	2 620
 # 619	
 # 620	    SCREEN_EPILOGUE (pScreen, CreateGC, miBSCreateGC);
	la	$25, $$845
	sw	$25, 272($3)
	.loc	2 622
 # 621	
 # 622	    return ret;
	move	$2, $5
	.endb	976
	lw	$31, 20($sp)
	addu	$sp, 32
	j	$31
	.end	miBSCreateGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 628
 # 623	}
 # 624	
 # 625	static Bool
 # 626	miBSDestroyWindow (pWin)
 # 627	    WindowPtr	pWin;
 # 628	{
	.ent	$$846 2
$$846:
	.option	O2
	subu	$sp, 32
	sw	$31, 20($sp)
	.mask	0x80000000, -12
	.frame	$sp, 32, $31
	.bgnb	980
	.loc	2 629
 # 629	    ScreenPtr	pScreen = pWin->drawable.pScreen;
	lw	$3, 16($4)
	.loc	2 632
 # 630	    Bool	ret;
 # 631	
 # 632	    SCREEN_PROLOGUE (pScreen, DestroyWindow);
	lw	$14, 404($3)
	lw	$15, $$839
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	lw	$9, 20($8)
	sw	$9, 160($3)
	.loc	2 634
 # 633	    
 # 634	    ret = (*pScreen->DestroyWindow) (pWin);
	sw	$3, 28($sp)
	sw	$4, 32($sp)
	lw	$10, 160($3)
	jal	$10
	lw	$3, 28($sp)
	lw	$4, 32($sp)
	sw	$2, 24($sp)
	.loc	2 636
 # 635	
 # 636	    miBSFree (pWin);
	jal	$$837
	lw	$3, 28($sp)
	.loc	2 638
 # 637	
 # 638	    SCREEN_EPILOGUE (pScreen, DestroyWindow, miBSDestroyWindow);
	la	$11, $$846
	sw	$11, 160($3)
	.loc	2 640
 # 639	
 # 640	    return ret;
	lw	$2, 24($sp)
	.endb	983
	lw	$31, 20($sp)
	addu	$sp, 32
	j	$31
	.end	miBSDestroyWindow
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 653
 # 653	{
	.ent	$$884 2
$$884:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	sw	$5, 28($sp)
	.bgnb	989
	.loc	2 654
 # 654	    CHEAP_FUNC_PROLOGUE (pGC);
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	sw	$8, 68($4)
	.loc	2 658
 # 655	    
 # 656	    if (pDrawable->type == DRAWABLE_WINDOW &&
 # 657	        ((WindowPtr) pDrawable)->backStorage != NULL &&
 # 658		miBSCreateGCPrivate (pGC))
	lbu	$9, 0($6)
	bne	$9, 0, $88
	lw	$10, 116($6)
	beq	$10, 0, $88
	sw	$4, 24($sp)
	sw	$6, 32($sp)
	jal	$$838
	lw	$4, 24($sp)
	lw	$6, 32($sp)
	beq	$2, 0, $88
	.loc	2 660
 # 659	    {
 # 660		(*pGC->funcs->ValidateGC) (pGC, stateChanges, pDrawable);
	lw	$5, 28($sp)
	lw	$11, 68($4)
	lw	$12, 0($11)
	jal	$12
	.loc	2 661
 # 661	    }
	b	$89
$88:
	.loc	2 664
 # 662	    else
 # 663	    {
 # 664		(*pGC->funcs->ValidateGC) (pGC, stateChanges, pDrawable);
	lw	$5, 28($sp)
	sw	$4, 24($sp)
	lw	$13, 68($4)
	lw	$15, 0($13)
	jal	$15
	lw	$4, 24($sp)
	.loc	2 665
 # 665		CHEAP_FUNC_EPILOGUE (pGC);
	la	$14, $$891
	sw	$14, 68($4)
	.loc	2 666
 # 666	    }
	.loc	2 667
 # 667	}
	.endb	990
$89:
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCheapValidateGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 673
 # 668	
 # 669	static void
 # 670	miBSCheapChangeGC (pGC, mask)
 # 671	    GCPtr   pGC;
 # 672	    unsigned long   mask;
 # 673	{
	.ent	$$887 2
$$887:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	995
	.loc	2 674
 # 674	    CHEAP_FUNC_PROLOGUE (pGC);
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	sw	$8, 68($4)
	.loc	2 676
 # 675	
 # 676	    (*pGC->funcs->ChangeGC) (pGC, mask);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 4($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 678
 # 677	
 # 678	    CHEAP_FUNC_EPILOGUE (pGC);
	la	$11, $$891
	sw	$11, 68($4)
	.loc	2 679
 # 679	}
	.endb	996
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCheapChangeGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 685
 # 680	
 # 681	static void
 # 682	miBSCheapCopyGC (pGCSrc, mask, pGCDst)
 # 683	    GCPtr   pGCSrc, pGCDst;
 # 684	    unsigned long   mask;
 # 685	{
	.ent	$$885 2
$$885:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1002
	.loc	2 686
 # 686	    CHEAP_FUNC_PROLOGUE (pGCDst);
	lw	$14, 76($6)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	sw	$8, 68($6)
	.loc	2 688
 # 687	
 # 688	    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
	sw	$6, 32($sp)
	lw	$9, 68($6)
	lw	$10, 8($9)
	jal	$10
	lw	$6, 32($sp)
	.loc	2 690
 # 689	
 # 690	    CHEAP_FUNC_EPILOGUE (pGCDst);
	la	$11, $$891
	sw	$11, 68($6)
	.loc	2 691
 # 691	}
	.endb	1003
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCheapCopyGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 696
 # 692	
 # 693	static void
 # 694	miBSCheapDestroyGC (pGC)
 # 695	    GCPtr   pGC;
 # 696	{
	.ent	$$886 2
$$886:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1007
	.loc	2 697
 # 697	    CHEAP_FUNC_PROLOGUE (pGC);
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	sw	$8, 68($4)
	.loc	2 699
 # 698	
 # 699	    (*pGC->funcs->DestroyGC) (pGC);
	lw	$9, 68($4)
	lw	$10, 12($9)
	jal	$10
	.loc	2 702
 # 700	
 # 701	    /* leave it unwrapped */
 # 702	}
	.endb	1008
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCheapDestroyGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 710
 # 710	{
	.ent	$$888 2
$$888:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1015
	.loc	2 711
 # 711	    CHEAP_FUNC_PROLOGUE (pGC);
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	sw	$8, 68($4)
	.loc	2 713
 # 712	
 # 713	    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 16($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 715
 # 714	
 # 715	    CHEAP_FUNC_EPILOGUE (pGC);
	la	$11, $$891
	sw	$11, 68($4)
	.loc	2 716
 # 716	}
	.endb	1016
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCheapChangeClip
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 721
 # 717	
 # 718	static void
 # 719	miBSCheapCopyClip(pgcDst, pgcSrc)
 # 720	    GCPtr pgcDst, pgcSrc;
 # 721	{
	.ent	$$890 2
$$890:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1021
	.loc	2 722
 # 722	    CHEAP_FUNC_PROLOGUE (pgcDst);
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	sw	$8, 68($4)
	.loc	2 724
 # 723	
 # 724	    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 24($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 726
 # 725	
 # 726	    CHEAP_FUNC_EPILOGUE (pgcDst);
	la	$11, $$891
	sw	$11, 68($4)
	.loc	2 727
 # 727	}
	.endb	1022
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCheapCopyClip
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 732
 # 728	
 # 729	static void
 # 730	miBSCheapDestroyClip(pGC)
 # 731	    GCPtr	pGC;
 # 732	{
	.ent	$$889 2
$$889:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1026
	.loc	2 733
 # 733	    CHEAP_FUNC_PROLOGUE (pGC);
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$8, 0($25)
	sw	$8, 68($4)
	.loc	2 735
 # 734	
 # 735	    (* pGC->funcs->DestroyClip)(pGC);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 20($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 737
 # 736	
 # 737	    CHEAP_FUNC_EPILOGUE (pGC);
	la	$11, $$891
	sw	$11, 68($4)
	.loc	2 738
 # 738	}
	.endb	1027
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCheapDestroyClip
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 747
 # 747	{
	.ent	$$838 2
$$838:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	move	$5, $4
	.bgnb	1031
	.loc	2 750
 # 748	    miBSGCRec	*pPriv;
 # 749	
 # 750	    pPriv = (miBSGCRec *) xalloc (sizeof (miBSGCRec));
	li	$4, 24
	sw	$5, 24($sp)
	jal	Xalloc
	lw	$5, 24($sp)
	move	$3, $2
	.loc	2 751
 # 751	    if (!pPriv)
	bne	$2, 0, $90
	.loc	2 752
 # 752		return FALSE;
	move	$2, $0
	b	$91
$90:
	.loc	2 753
 # 753	    pPriv->pBackingGC = NULL;
	sw	$0, 0($3)
	.loc	2 754
 # 754	    pPriv->guarantee = GuaranteeNothing;
	sw	$0, 4($3)
	.loc	2 755
 # 755	    pPriv->serialNumber = 0;
	sw	$0, 8($3)
	.loc	2 756
 # 756	    pPriv->stateChanges = (1 << GCLastBit + 1) - 1;
	li	$14, 8388607
	sw	$14, 12($3)
	.loc	2 757
 # 757	    pPriv->wrapOps = pGC->ops;
	lw	$15, 72($5)
	sw	$15, 16($3)
	.loc	2 758
 # 758	    pPriv->wrapFuncs = pGC->funcs;
	lw	$24, 68($5)
	sw	$24, 20($3)
	.loc	2 759
 # 759	    pGC->funcs = &miBSGCFuncs;
	la	$25, $$861
	sw	$25, 68($5)
	.loc	2 760
 # 760	    pGC->ops = &miBSGCOps;
	la	$8, $$883
	sw	$8, 72($5)
	.loc	2 761
 # 761	    pGC->devPrivates[miBSGCIndex].ptr = (pointer) pPriv;
	lw	$9, 76($5)
	lw	$10, $$853
	mul	$11, $10, 4
	addu	$12, $9, $11
	sw	$3, 0($12)
	.loc	2 762
 # 762	    return TRUE;
	li	$2, 1
	.endb	1033
$91:
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCreateGCPrivate
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 768
 # 763	}
 # 764	
 # 765	static void
 # 766	miBSDestroyGCPrivate (pGC)
 # 767	    GCPtr   pGC;
 # 768	{
	.ent	$$1036 2
$$1036:
	.option	O2
	subu	$sp, 32
	sw	$31, 20($sp)
	.mask	0x80000000, -12
	.frame	$sp, 32, $31
	.bgnb	1038
	.loc	2 771
 # 769	    miBSGCRec	*pPriv;
 # 770	
 # 771	    pPriv = (miBSGCRec *) pGC->devPrivates[miBSGCIndex].ptr;
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$2, $14, $24
	lw	$3, 0($2)
	.loc	2 772
 # 772	    if (pPriv)
	beq	$3, 0, $93
	.loc	2 774
 # 773	    {
 # 774		pGC->devPrivates[miBSGCIndex].ptr = (pointer) pPriv->wrapFuncs;
	lw	$25, 20($3)
	sw	$25, 0($2)
	.loc	2 775
 # 775		pGC->funcs = &miBSCheapGCFuncs;
	la	$8, $$891
	sw	$8, 68($4)
	.loc	2 776
 # 776		pGC->ops = pPriv->wrapOps;
	lw	$9, 16($3)
	sw	$9, 72($4)
	.loc	2 777
 # 777		if (pPriv->pBackingGC)
	lw	$6, 0($3)
	beq	$6, $0, $92
	.loc	2 778
 # 778		    FreeGC (pPriv->pBackingGC, (GContext) 0);
	move	$4, $6
	move	$5, $0
	sw	$3, 28($sp)
	jal	FreeGC
	lw	$3, 28($sp)
$92:
	.loc	2 779
 # 779		xfree ((pointer) pPriv);
	move	$4, $3
	jal	Xfree
	.loc	2 780
 # 780	    }
	.loc	2 781
 # 781	}
	.endb	1040
$93:
	lw	$31, 20($sp)
	addu	$sp, 32
	j	$31
	.end	miBSDestroyGCPrivate
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 807
 # 807	{
	.ent	$$862 2
$$862:
	.option	O2
	subu	$sp, 88
	sw	$31, 36($sp)
	sd	$16, 28($sp)
	.mask	0x80030000, -52
	.frame	$sp, 88, $31
	sw	$4, 88($sp)
	move	$16, $5
	sw	$6, 96($sp)
	sw	$7, 100($sp)
	.bgnb	1048
	.loc	2 810
 # 808	    DDXPointPtr	pptCopy, pptReset;
 # 809	    int 	*pwidthCopy;
 # 810	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 88($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 72($sp)
	.loc	2 810
	lw	$25, 76($16)
	lw	$10, $$853
	mul	$11, $10, 4
	addu	$12, $25, $11
	lw	$2, 0($12)
	.loc	2 810
	lw	$13, 68($16)
	sw	$13, 64($sp)
	.loc	2 810
	lw	$14, 0($2)
	sw	$14, 60($sp)
	.loc	2 812
 # 811	
 # 812	    PROLOGUE(pGC);
	lw	$15, 16($2)
	sw	$15, 72($16)
	.loc	2 812
	lw	$24, 20($2)
	sw	$24, 68($16)
	.loc	2 812
	.loc	2 814
 # 813	
 # 814	    pptCopy = (DDXPointPtr)ALLOCATE_LOCAL(nInit*sizeof(DDXPointRec));
	lw	$10, 96($sp)
	mul	$25, $10, 4
	sw	$25, 40($sp)
	move	$4, $25
	sw	$2, 68($sp)
	jal	Xalloc
	move	$17, $2
	.loc	2 815
 # 815	    pwidthCopy=(int *)ALLOCATE_LOCAL(nInit*sizeof(int));
	lw	$4, 40($sp)
	jal	Xalloc
	sw	$2, 76($sp)
	.loc	2 816
 # 816	    if (pptCopy && pwidthCopy)
	beq	$17, 0, $97
	beq	$2, 0, $97
	.loc	2 818
 # 817	    {
 # 818		bcopy((char *)pptInit,(char *)pptCopy,nInit*sizeof(DDXPointRec));
	lw	$4, 100($sp)
	move	$5, $17
	lw	$6, 40($sp)
	jal	bcopy
	.loc	2 819
 # 819		bcopy((char *)pwidthInit,(char *)pwidthCopy,nInit*sizeof(int));
	lw	$4, 104($sp)
	lw	$5, 76($sp)
	lw	$6, 40($sp)
	jal	bcopy
	.loc	2 822
 # 820	
 # 821		(* pGC->ops->FillSpans)(pDrawable, pGC, nInit, pptInit,
 # 822				     pwidthInit, fSorted);
	lw	$4, 88($sp)
	move	$5, $16
	lw	$6, 96($sp)
	lw	$7, 100($sp)
	lw	$11, 104($sp)
	sw	$11, 16($sp)
	lw	$12, 108($sp)
	sw	$12, 20($sp)
	lw	$13, 72($16)
	lw	$14, 0($13)
	jal	$14
	.loc	2 823
 # 823		if (pGC->miTranslate)
	lw	$15, 16($16)
	sll	$24, $15, 17
	srl	$10, $24, 31
	bne	$10, $0, $94
	lw	$9, 96($sp)
	lw	$8, 72($sp)
	b	$96
$94:
	lw	$8, 72($sp)
	lw	$9, 96($sp)
	.bgnb	1057
	.loc	2 828
 # 824		{
 # 825		    int	dx, dy;
 # 826		    int	nReset;
 # 827	
 # 828		    pptReset = pptCopy;
	move	$2, $17
	.loc	2 829
 # 829		    dx = pDrawable->x - pBackingDrawable->x;
	lw	$25, 88($sp)
	lh	$11, 8($25)
	lh	$12, 8($8)
	subu	$5, $11, $12
	.loc	2 830
 # 830		    dy = pDrawable->y - pBackingDrawable->y;
	lh	$13, 10($25)
	lh	$14, 10($8)
	subu	$6, $13, $14
	.loc	2 831
 # 831		    nReset = nInit;
	.loc	2 832
 # 832		    while (nReset--)
	addu	$4, $9, -1
	beq	$9, 0, $96
$95:
	.loc	2 834
 # 833		    {
 # 834			pptReset->x -= dx;
	lh	$15, 0($2)
	subu	$24, $15, $5
	sh	$24, 0($2)
	.loc	2 835
 # 835			pptReset->y -= dy;
	lh	$10, 2($2)
	subu	$11, $10, $6
	sh	$11, 2($2)
	.loc	2 836
 # 836			++pptReset;
	addu	$2, $2, 4
	.loc	2 837
 # 837		    }
	.loc	2 837
	move	$3, $4
	addu	$4, $4, -1
	bne	$3, 0, $95
	.loc	2 838
 # 838		}
	.endb	1061
$96:
	lw	$5, 60($sp)
	.loc	2 841
 # 839		(* pBackingGC->ops->FillSpans)(pBackingDrawable,
 # 840					  pBackingGC, nInit, pptCopy, pwidthCopy,
 # 841					  fSorted);
	move	$4, $8
	move	$6, $9
	move	$7, $17
	lw	$12, 76($sp)
	sw	$12, 16($sp)
	lw	$25, 108($sp)
	sw	$25, 20($sp)
	lw	$13, 72($5)
	lw	$14, 0($13)
	jal	$14
	.loc	2 842
 # 842	    }
$97:
	.loc	2 843
 # 843	    if (pwidthCopy) DEALLOCATE_LOCAL(pwidthCopy);
	lw	$15, 76($sp)
	beq	$15, 0, $98
	.loc	2 843
	move	$4, $15
	jal	Xfree
$98:
	.loc	2 844
 # 844	    if (pptCopy) DEALLOCATE_LOCAL(pptCopy);
	beq	$17, 0, $99
	.loc	2 844
	move	$4, $17
	jal	Xfree
$99:
	.loc	2 846
 # 845	
 # 846	    EPILOGUE (pGC);
	lw	$24, 72($16)
	lw	$10, 68($sp)
	sw	$24, 16($10)
	.loc	2 846
	la	$11, $$883
	sw	$11, 72($16)
	.loc	2 846
	lw	$12, 64($sp)
	sw	$12, 68($16)
	.loc	2 846
	.loc	2 847
 # 847	}
	.endb	1062
	ld	$16, 28($sp)
	lw	$31, 36($sp)
	addu	$sp, 88
	j	$31
	.end	miBSFillSpans
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 870
 # 870	{
	.ent	$$863 2
$$863:
	.option	O2
	subu	$sp, 96
	sw	$31, 44($sp)
	sd	$16, 36($sp)
	.mask	0x80030000, -52
	.frame	$sp, 96, $31
	sw	$4, 96($sp)
	move	$16, $5
	sw	$6, 104($sp)
	sw	$7, 108($sp)
	.bgnb	1072
	.loc	2 873
 # 871	    DDXPointPtr	pptCopy, pptReset;
 # 872	    int 	*pwidthCopy;
 # 873	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 96($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 80($sp)
	.loc	2 873
	lw	$25, 76($16)
	lw	$10, $$853
	mul	$11, $10, 4
	addu	$12, $25, $11
	lw	$2, 0($12)
	.loc	2 873
	lw	$13, 68($16)
	sw	$13, 72($sp)
	.loc	2 873
	lw	$14, 0($2)
	sw	$14, 68($sp)
	.loc	2 875
 # 874	
 # 875	    PROLOGUE(pGC);
	lw	$15, 16($2)
	sw	$15, 72($16)
	.loc	2 875
	lw	$24, 20($2)
	sw	$24, 68($16)
	.loc	2 875
	.loc	2 877
 # 876	
 # 877	    pptCopy = (DDXPointPtr)ALLOCATE_LOCAL(nspans*sizeof(DDXPointRec));
	lw	$10, 116($sp)
	mul	$25, $10, 4
	sw	$25, 48($sp)
	move	$4, $25
	sw	$2, 76($sp)
	jal	Xalloc
	move	$17, $2
	.loc	2 878
 # 878	    pwidthCopy=(int *)ALLOCATE_LOCAL(nspans*sizeof(int));
	lw	$4, 48($sp)
	jal	Xalloc
	sw	$2, 84($sp)
	.loc	2 879
 # 879	    if (pptCopy && pwidthCopy)
	beq	$17, 0, $103
	beq	$2, 0, $103
	.loc	2 881
 # 880	    {
 # 881		bcopy((char *)ppt,(char *)pptCopy,nspans*sizeof(DDXPointRec));
	lw	$4, 108($sp)
	move	$5, $17
	lw	$6, 48($sp)
	jal	bcopy
	.loc	2 882
 # 882		bcopy((char *)pwidth,(char *)pwidthCopy,nspans*sizeof(int));
	lw	$4, 112($sp)
	lw	$5, 84($sp)
	lw	$6, 48($sp)
	jal	bcopy
	.loc	2 885
 # 883	
 # 884		(* pGC->ops->SetSpans)(pDrawable, pGC, psrc, ppt, pwidth,
 # 885				    nspans, fSorted);
	lw	$4, 96($sp)
	move	$5, $16
	lw	$6, 104($sp)
	lw	$7, 108($sp)
	lw	$11, 112($sp)
	sw	$11, 16($sp)
	lw	$12, 116($sp)
	sw	$12, 20($sp)
	lw	$13, 120($sp)
	sw	$13, 24($sp)
	lw	$14, 72($16)
	lw	$15, 4($14)
	jal	$15
	.loc	2 886
 # 886		if (pGC->miTranslate)
	lw	$24, 16($16)
	sll	$10, $24, 17
	srl	$25, $10, 31
	bne	$25, $0, $100
	lw	$9, 116($sp)
	lw	$8, 80($sp)
	b	$102
$100:
	lw	$8, 80($sp)
	lw	$9, 116($sp)
	.bgnb	1080
	.loc	2 891
 # 887		{
 # 888		    int	dx, dy;
 # 889		    int	nReset;
 # 890	
 # 891		    pptReset = pptCopy;
	move	$2, $17
	.loc	2 892
 # 892		    dx = pDrawable->x - pBackingDrawable->x;
	lw	$11, 96($sp)
	lh	$12, 8($11)
	lh	$13, 8($8)
	subu	$5, $12, $13
	.loc	2 893
 # 893		    dy = pDrawable->y - pBackingDrawable->y;
	lh	$14, 10($11)
	lh	$15, 10($8)
	subu	$6, $14, $15
	.loc	2 894
 # 894		    nReset = nspans;
	.loc	2 895
 # 895		    while (nReset--)
	addu	$4, $9, -1
	beq	$9, 0, $102
$101:
	.loc	2 897
 # 896		    {
 # 897			pptReset->x -= dx;
	lh	$24, 0($2)
	subu	$10, $24, $5
	sh	$10, 0($2)
	.loc	2 898
 # 898			pptReset->y -= dy;
	lh	$25, 2($2)
	subu	$12, $25, $6
	sh	$12, 2($2)
	.loc	2 899
 # 899			++pptReset;
	addu	$2, $2, 4
	.loc	2 900
 # 900		    }
	.loc	2 900
	move	$3, $4
	addu	$4, $4, -1
	bne	$3, 0, $101
	.loc	2 901
 # 901		}
	.endb	1084
$102:
	lw	$5, 68($sp)
	.loc	2 904
 # 902		(* pBackingGC->ops->SetSpans)(pBackingDrawable, pBackingGC,
 # 903					 psrc, pptCopy, pwidthCopy, nspans,
 # 904					 fSorted);
	move	$4, $8
	lw	$6, 104($sp)
	move	$7, $17
	lw	$13, 84($sp)
	sw	$13, 16($sp)
	sw	$9, 20($sp)
	lw	$11, 120($sp)
	sw	$11, 24($sp)
	lw	$14, 72($5)
	lw	$15, 4($14)
	jal	$15
	.loc	2 905
 # 905	    }
$103:
	.loc	2 906
 # 906	    if (pwidthCopy) DEALLOCATE_LOCAL(pwidthCopy);
	lw	$24, 84($sp)
	beq	$24, 0, $104
	.loc	2 906
	move	$4, $24
	jal	Xfree
$104:
	.loc	2 907
 # 907	    if (pptCopy) DEALLOCATE_LOCAL(pptCopy);
	beq	$17, 0, $105
	.loc	2 907
	move	$4, $17
	jal	Xfree
$105:
	.loc	2 909
 # 908	
 # 909	    EPILOGUE (pGC);
	lw	$10, 72($16)
	lw	$25, 76($sp)
	sw	$10, 16($25)
	.loc	2 909
	la	$12, $$883
	sw	$12, 72($16)
	.loc	2 909
	lw	$13, 72($sp)
	sw	$13, 68($16)
	.loc	2 909
	.loc	2 910
 # 910	}
	.endb	1085
	ld	$16, 36($sp)
	lw	$31, 44($sp)
	addu	$sp, 96
	j	$31
	.end	miBSSetSpans
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 935
 # 935	{
	.ent	$$864 2
$$864:
	.option	O2
	subu	$sp, 64
	sw	$31, 44($sp)
	.mask	0x80000000, -20
	.frame	$sp, 64, $31
	move	$3, $5
	sw	$6, 72($sp)
	sw	$7, 76($sp)
	.bgnb	1098
	.loc	2 936
 # 936	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 116($4)
	lw	$15, 0($14)
	sw	$15, 60($sp)
	.loc	2 936
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$9, $25, 4
	addu	$10, $24, $9
	lw	$2, 0($10)
	.loc	2 936
	lw	$11, 68($3)
	sw	$11, 52($sp)
	.loc	2 936
	lw	$8, 0($2)
	.loc	2 938
 # 937	
 # 938	    PROLOGUE(pGC);
	lw	$12, 16($2)
	sw	$12, 72($3)
	.loc	2 938
	lw	$13, 20($2)
	sw	$13, 68($3)
	.loc	2 938
	.loc	2 941
 # 939	
 # 940	    (*pGC->ops->PutImage)(pDrawable, pGC,
 # 941			     depth, x, y, w, h, leftPad, format, pBits);
	move	$5, $3
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$14, 80($sp)
	sw	$14, 16($sp)
	lw	$15, 84($sp)
	sw	$15, 20($sp)
	lw	$25, 88($sp)
	sw	$25, 24($sp)
	lw	$24, 92($sp)
	sw	$24, 28($sp)
	lw	$9, 96($sp)
	sw	$9, 32($sp)
	lw	$10, 100($sp)
	sw	$10, 36($sp)
	sw	$2, 56($sp)
	sw	$3, 68($sp)
	sw	$8, 48($sp)
	lw	$11, 72($3)
	lw	$12, 8($11)
	jal	$12
	lw	$2, 56($sp)
	lw	$3, 68($sp)
	lw	$8, 48($sp)
	.loc	2 943
 # 942	    (*pBackingGC->ops->PutImage)(pBackingDrawable, pBackingGC,
 # 943			     depth, x, y, w, h, leftPad, format, pBits);
	lw	$4, 60($sp)
	move	$5, $8
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$13, 80($sp)
	sw	$13, 16($sp)
	lw	$14, 84($sp)
	sw	$14, 20($sp)
	lw	$15, 88($sp)
	sw	$15, 24($sp)
	lw	$25, 92($sp)
	sw	$25, 28($sp)
	lw	$24, 96($sp)
	sw	$24, 32($sp)
	lw	$9, 100($sp)
	sw	$9, 36($sp)
	lw	$10, 72($8)
	lw	$11, 8($10)
	jal	$11
	lw	$2, 56($sp)
	lw	$3, 68($sp)
	.loc	2 945
 # 944	
 # 945	    EPILOGUE (pGC);
	lw	$12, 72($3)
	sw	$12, 16($2)
	.loc	2 945
	la	$13, $$883
	sw	$13, 72($3)
	.loc	2 945
	lw	$14, 52($sp)
	sw	$14, 68($3)
	.loc	2 945
	.loc	2 946
 # 946	}
	.endb	1103
	lw	$31, 44($sp)
	addu	$sp, 64
	j	$31
	.end	miBSPutImage
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 988
 # 988	{
	.ent	$$1106 2
$$1106:
	.option	O2
	subu	$sp, 216
	sw	$31, 52($sp)
	sw	$23, 48($sp)
	sw	$16, 44($sp)
	.mask	0x80810000, -164
	.frame	$sp, 216, $31
	move	$23, $4
	move	$3, $5
	sw	$6, 224($sp)
	sw	$7, 228($sp)
	.bgnb	1118
	.loc	2 1008
 #1008	    SETUP_BACKING_VERBOSE (pWin, pGC);
	lw	$16, 116($23)
	.loc	2 1008
	lw	$14, 0($16)
	sw	$14, 124($sp)
	.loc	2 1008
	lw	$15, 76($3)
	lw	$24, $$853
	mul	$25, $24, 4
	addu	$13, $15, $25
	lw	$2, 0($13)
	.loc	2 1008
	.loc	2 1008
	lw	$14, 0($2)
	sw	$14, 112($sp)
	.loc	2 1013
 #1009	
 #1010	    /*
 #1011	     * Create a region of exposed boxes in pRgnExp.
 #1012	     */
 #1013	    box.x1 = srcx + pWin->drawable.x;
	lw	$24, 224($sp)
	lh	$15, 8($23)
	addu	$25, $24, $15
	sh	$25, 200($sp)
	.loc	2 1014
 #1014	    box.x2 = box.x1 + w;
	lw	$13, 232($sp)
	sll	$14, $25, 16
	sra	$24, $14, 16
	addu	$15, $24, $13
	sh	$15, 204($sp)
	.loc	2 1015
 #1015	    box.y1 = srcy + pWin->drawable.y;
	lw	$25, 228($sp)
	lh	$14, 10($23)
	addu	$24, $25, $14
	sh	$24, 202($sp)
	.loc	2 1016
 #1016	    box.y2 = box.y1 + h;
	lw	$13, 236($sp)
	sll	$15, $24, 16
	sra	$25, $15, 16
	addu	$14, $25, $13
	sh	$14, 206($sp)
	.loc	2 1018
 #1017	    
 #1018	    pRgnExp = (*pGC->pScreen->RegionCreate) (&box, 1);
	addu	$4, $sp, 200
	li	$5, 1
	sw	$3, 220($sp)
	lw	$24, 0($3)
	lw	$15, 304($24)
	jal	$15
	sw	$2, 212($sp)
	.loc	2 1019
 #1019	    (*pGC->pScreen->Intersect) (pRgnExp, pRgnExp, &pWin->clipList);
	move	$4, $2
	move	$5, $2
	addu	$6, $23, 44
	lw	$25, 220($sp)
	lw	$13, 0($25)
	lw	$14, 324($13)
	jal	$14
	.loc	2 1020
 #1020	    pRgnObs = (*pGC->pScreen->RegionCreate) (NULL, 1);
	move	$4, $0
	li	$5, 1
	lw	$24, 220($sp)
	lw	$15, 0($24)
	lw	$25, 304($15)
	jal	$25
	sw	$2, 208($sp)
	.loc	2 1021
 #1021	    (* pGC->pScreen->Inverse) (pRgnObs, pRgnExp, &box);
	move	$4, $2
	lw	$5, 212($sp)
	addu	$6, $sp, 200
	lw	$13, 220($sp)
	lw	$14, 0($13)
	lw	$24, 336($14)
	jal	$24
	.loc	2 1030
 #1030					      -pWin->drawable.y);
	lw	$4, 212($sp)
	lh	$5, 8($23)
	negu	$5, $5
	lh	$6, 10($23)
	negu	$6, $6
	lw	$15, 220($sp)
	lw	$25, 0($15)
	lw	$13, 344($25)
	jal	$13
	.loc	2 1033
 #1031	    (*pGC->pScreen->TranslateRegion) (pRgnObs,
 #1032					      -pWin->drawable.x,
 #1033					      -pWin->drawable.y);
	lw	$4, 208($sp)
	lh	$5, 8($23)
	negu	$5, $5
	lh	$6, 10($23)
	negu	$6, $6
	lw	$14, 220($sp)
	lw	$24, 0($14)
	lw	$15, 344($24)
	jal	$15
	lw	$4, 208($sp)
	.loc	2 1034
 #1034	    (*pGC->pScreen->Intersect)(pRgnObs, pRgnObs, &pBackingStore->SavedRegion);
	move	$5, $4
	addu	$6, $16, 4
	lw	$25, 220($sp)
	lw	$13, 0($25)
	lw	$14, 324($13)
	jal	$14
	.loc	2 1039
 #1035	
 #1036	    /*
 #1037	     * If the obscured region is empty, there's no point being fancy.
 #1038	     */
 #1039	    if (!(*pGC->pScreen->RegionNotEmpty) (pRgnObs))
	lw	$4, 208($sp)
	lw	$24, 220($sp)
	lw	$15, 0($24)
	lw	$25, 356($15)
	jal	$25
	bne	$2, 0, $106
	.loc	2 1041
 #1040	    {
 #1041		(*pGC->pScreen->RegionDestroy) (pRgnExp);
	lw	$4, 212($sp)
	lw	$13, 220($sp)
	lw	$14, 0($13)
	lw	$24, 316($14)
	jal	$24
	.loc	2 1042
 #1042		(*pGC->pScreen->RegionDestroy) (pRgnObs);
	lw	$4, 208($sp)
	lw	$15, 220($sp)
	lw	$25, 0($15)
	lw	$13, 316($25)
	jal	$13
	.loc	2 1044
 #1043	
 #1044		return (FALSE);
	move	$2, $0
	b	$156
$106:
	lw	$3, 212($sp)
	.loc	2 1047
 #1045	    }
 #1046	
 #1047	    numRectsExp = REGION_NUM_RECTS(pRgnExp);
	lw	$2, 8($3)
	sw	$17, 84($sp)
	sw	$18, 80($sp)
	sw	$19, 76($sp)
	sw	$22, 72($sp)
	sw	$30, 68($sp)
	beq	$2, $0, $107
	lw	$18, 4($2)
	b	$108
$107:
	li	$18, 1
$108:
	.loc	2 1048
 #1048	    pBoxExp = REGION_RECTS(pRgnExp);
	beq	$2, $0, $109
	addu	$17, $2, 8
	b	$110
$109:
	move	$17, $3
$110:
	lw	$5, 208($sp)
	.loc	2 1049
 #1049	    pBoxObs = REGION_RECTS(pRgnObs);
	lw	$2, 8($5)
	beq	$2, $0, $111
	addu	$19, $2, 8
	b	$112
$111:
	move	$19, $5
$112:
	.loc	2 1050
 #1050	    numRectsObs = REGION_NUM_RECTS(pRgnObs);
	beq	$2, $0, $113
	lw	$16, 4($2)
	b	$114
$113:
	li	$16, 1
$114:
	.loc	2 1051
 #1051	    nrects = numRectsExp + numRectsObs;
	addu	$22, $18, $16
	.loc	2 1053
 #1052	    
 #1053	    boxes = (struct BoxDraw *)ALLOCATE_LOCAL(nrects * sizeof(struct BoxDraw));
	mul	$4, $22, 8
	jal	Xalloc
	move	$30, $2
	.loc	2 1054
 #1054	    sequence = (int *) ALLOCATE_LOCAL(nrects * sizeof(int));
	mul	$4, $22, 4
	jal	Xalloc
	sw	$2, 192($sp)
	.loc	2 1055
 #1055	    *ppRgn = NULL;
	lw	$14, 256($sp)
	sw	$0, 0($14)
	.loc	2 1057
 #1056	
 #1057	    if (!boxes || !sequence)
	beq	$30, 0, $115
	bne	$2, 0, $118
$115:
	.loc	2 1059
 #1058	    {
 #1059		if (sequence) DEALLOCATE_LOCAL(sequence);
	beq	$2, 0, $116
	.loc	2 1059
	lw	$4, 192($sp)
	jal	Xfree
$116:
	.loc	2 1060
 #1060		if (boxes) DEALLOCATE_LOCAL(boxes);
	beq	$30, 0, $117
	.loc	2 1060
	move	$4, $30
	jal	Xfree
$117:
	.loc	2 1061
 #1061		(*pGC->pScreen->RegionDestroy) (pRgnExp);
	lw	$4, 212($sp)
	lw	$24, 220($sp)
	lw	$15, 0($24)
	lw	$25, 316($15)
	jal	$25
	.loc	2 1062
 #1062		(*pGC->pScreen->RegionDestroy) (pRgnObs);
	lw	$4, 208($sp)
	lw	$13, 220($sp)
	lw	$14, 0($13)
	lw	$24, 316($14)
	jal	$24
	.loc	2 1064
 #1063	
 #1064		return(TRUE);
	li	$2, 1
	lw	$17, 84($sp)
	lw	$18, 80($sp)
	lw	$19, 76($sp)
	lw	$22, 72($sp)
	lw	$30, 68($sp)
	b	$156
$118:
	.loc	2 1072
 #1072		 (i < numRectsExp) && (j < numRectsObs);
	move	$8, $0
	move	$4, $0
	move	$9, $0
	.loc	2 1072
	sw	$20, 64($sp)
	sw	$21, 60($sp)
	ble	$18, 0, $124
	ble	$16, 0, $124
	mul	$15, $8, 8
	addu	$7, $17, $15
	mul	$25, $4, 8
	addu	$10, $19, $25
	mul	$13, $9, 8
	addu	$6, $30, $13
$119:
	.loc	2 1075
 #1073		 k++)
 #1074	    {
 #1075		if (pBoxExp[i].y1 < pBoxObs[j].y1)
	lh	$3, 2($7)
	lh	$5, 2($10)
	bge	$3, $5, $120
	.loc	2 1077
 #1076		{
 #1077		    boxes[k].pBox = &pBoxExp[i];
	mul	$14, $8, 8
	addu	$24, $17, $14
	sw	$24, 0($6)
	.loc	2 1078
 #1078		    boxes[k].source = win;
	sw	$0, 4($6)
	.loc	2 1079
 #1079		    i++;
	addu	$8, $8, 1
	addu	$7, $7, 8
	.loc	2 1080
 #1080		}
	b	$123
$120:
	.loc	2 1082
 #1081		else if ((pBoxObs[j].y1 < pBoxExp[i].y1) ||
 #1082			 (pBoxObs[j].x1 < pBoxExp[i].x1))
	blt	$5, $3, $121
	lh	$15, 0($10)
	lh	$25, 0($7)
	bge	$15, $25, $122
$121:
	.loc	2 1084
 #1083		{
 #1084		    boxes[k].pBox = &pBoxObs[j];
	mul	$13, $4, 8
	addu	$14, $19, $13
	sw	$14, 0($6)
	.loc	2 1085
 #1085		    boxes[k].source = pix;
	li	$24, 1
	sw	$24, 4($6)
	.loc	2 1086
 #1086		    j++;
	addu	$4, $4, 1
	addu	$10, $10, 8
	.loc	2 1087
 #1087		}
	b	$123
$122:
	.loc	2 1090
 #1088		else
 #1089		{
 #1090		    boxes[k].pBox = &pBoxExp[i];
	mul	$15, $8, 8
	addu	$25, $17, $15
	sw	$25, 0($6)
	.loc	2 1091
 #1091		    boxes[k].source = win;
	sw	$0, 4($6)
	.loc	2 1092
 #1092		    i++;
	addu	$8, $8, 1
	addu	$7, $7, 8
	.loc	2 1093
 #1093		}
$123:
	.loc	2 1094
 #1094	    }
	.loc	2 1094
	addu	$9, $9, 1
	addu	$6, $6, 8
	.loc	2 1094
	bge	$8, $18, $124
	blt	$4, $16, $119
$124:
	.loc	2 1100
 #1095	
 #1096	    /*
 #1097	     * Catch any leftover boxes from either region (note that only
 #1098	     * one can have leftover boxes...)
 #1099	     */
 #1100	    if (i != numRectsExp)
	mul	$13, $9, 8
	addu	$6, $30, $13
	beq	$8, $18, $126
	mul	$14, $8, 8
	addu	$3, $17, $14
$125:
	.loc	2 1104
 #1101	    {
 #1102		do
 #1103		{
 #1104		    boxes[k].pBox = &pBoxExp[i];
	sw	$3, 0($6)
	.loc	2 1105
 #1105		    boxes[k].source = win;
	sw	$0, 4($6)
	.loc	2 1106
 #1106		    i++;
	addu	$8, $8, 1
	addu	$3, $3, 8
	.loc	2 1107
 #1107		    k++;
	addu	$6, $6, 8
	.loc	2 1108
 #1108		} while (i < numRectsExp);
	.loc	2 1108
	blt	$8, $18, $125
	.loc	2 1110
 #1109	
 #1110	    }
	b	$128
$126:
	mul	$24, $4, 8
	addu	$3, $19, $24
$127:
	.loc	2 1115
 #1111	    else
 #1112	    {
 #1113		do
 #1114		{
 #1115		    boxes[k].pBox = &pBoxObs[j];
	sw	$3, 0($6)
	.loc	2 1116
 #1116		    boxes[k].source = pix;
	li	$15, 1
	sw	$15, 4($6)
	.loc	2 1117
 #1117		    j++;
	addu	$4, $4, 1
	addu	$3, $3, 8
	.loc	2 1118
 #1118		    k++;
	addu	$6, $6, 8
	.loc	2 1119
 #1119		} while (j < numRectsObs);
	.loc	2 1119
	blt	$4, $16, $127
	.loc	2 1120
 #1120	    }
$128:
	lw	$12, 244($sp)
	.loc	2 1122
 #1121	    
 #1122	    if (dsty <= srcy)
	lw	$25, 228($sp)
	bgt	$12, $25, $136
	lw	$11, 240($sp)
	.loc	2 1127
 #1123	    {
 #1124		/*
 #1125		 * Scroll up or vertically stationary, so vertical order is ok.
 #1126		 */
 #1127		if (dstx <= srcx)
	move	$8, $0
	lw	$13, 224($sp)
	bgt	$11, $13, $130
	.loc	2 1133
 #1128		{
 #1129		    /*
 #1130		     * Scroll left or horizontally stationary, so horizontal order
 #1131		     * is ok as well.
 #1132		     */
 #1133		    for (i = 0; i < nrects; i++)
	.loc	2 1133
	ble	$22, 0, $145
	move	$3, $2
$129:
	.loc	2 1135
 #1134		    {
 #1135			sequence[i] = i;
	sw	$8, 0($3)
	.loc	2 1136
 #1136		    }
	.loc	2 1136
	addu	$8, $8, 1
	addu	$3, $3, 4
	.loc	2 1136
	blt	$8, $22, $129
	move	$8, $0
	.loc	2 1137
 #1137		}
	b	$145
$130:
	.loc	2 1145
 #1145			 i < nrects;
	li	$4, 1
	move	$9, $0
	.loc	2 1145
	ble	$22, 0, $145
	move	$6, $30
$131:
	.loc	2 1148
 #1146			 j = i + 1, k = i)
 #1147		    {
 #1148			y = boxes[i].pBox->y1;
	lw	$14, 0($6)
	lh	$5, 2($14)
	.loc	2 1149
 #1149			while ((j < nrects) && (boxes[j].pBox->y1 == y))
	bge	$4, $22, $133
	mul	$24, $4, 8
	addu	$3, $30, $24
	lw	$15, 0($3)
	lh	$25, 2($15)
	bne	$25, $5, $133
$132:
	.loc	2 1151
 #1150			{
 #1151			    j++;
	addu	$4, $4, 1
	addu	$3, $3, 8
	.loc	2 1152
 #1152			}
	.loc	2 1152
	bge	$4, $22, $133
	lw	$13, 0($3)
	lh	$14, 2($13)
	beq	$14, $5, $132
$133:
	.loc	2 1153
 #1153			for (j--; j >= k; j--, i++)
	addu	$4, $4, -1
	.loc	2 1153
	blt	$4, $9, $135
	mul	$24, $8, 4
	addu	$3, $2, $24
$134:
	.loc	2 1155
 #1154			{
 #1155			    sequence[i] = j;
	sw	$4, 0($3)
	.loc	2 1156
 #1156			}
	.loc	2 1156
	addu	$4, $4, -1
	addu	$8, $8, 1
	addu	$6, $6, 8
	addu	$3, $3, 4
	.loc	2 1156
	bge	$4, $9, $134
$135:
	.loc	2 1157
 #1157		    }
	.loc	2 1157
	addu	$4, $8, 1
	move	$9, $8
	.loc	2 1157
	blt	$8, $22, $131
	move	$8, $0
	.loc	2 1158
 #1158		}
	.loc	2 1159
 #1159	    }
	b	$145
$136:
	lw	$11, 240($sp)
	.loc	2 1165
 #1160	    else
 #1161	    {
 #1162		/*
 #1163		 * Scroll down. Must reverse vertical banding, at least.
 #1164		 */
 #1165		if (dstx < srcx)
	lw	$15, 224($sp)
	bge	$11, $15, $143
	.loc	2 1171
 #1166		{
 #1167		    /*
 #1168		     * Scroll left. Horizontal order is ok.
 #1169		     */
 #1170		    for (i = nrects - 1, j = i - 1, k = i, l = 0;
 #1171			 i >= 0;
	addu	$8, $22, -1
	addu	$10, $8, -1
	move	$4, $10
	move	$9, $8
	move	$7, $0
	.loc	2 1171
	blt	$8, 0, $142
	mul	$25, $8, 8
	addu	$6, $30, $25
$137:
	.loc	2 1178
 #1178			y = boxes[i].pBox->y1;
	lw	$13, 0($6)
	lh	$5, 2($13)
	.loc	2 1179
 #1179			while ((j >= 0) && (boxes[j].pBox->y1 == y))
	blt	$4, 0, $139
	mul	$14, $4, 8
	addu	$3, $30, $14
	lw	$24, 0($3)
	lh	$15, 2($24)
	bne	$15, $5, $139
$138:
	.loc	2 1181
 #1180			{
 #1181			    j--;
	addu	$4, $4, -1
	addu	$3, $3, -8
	.loc	2 1182
 #1182			}
	.loc	2 1182
	blt	$4, 0, $139
	lw	$25, 0($3)
	lh	$13, 2($25)
	beq	$13, $5, $138
$139:
	.loc	2 1183
 #1183			for (j++; j <= k; j++, i--, l++)
	addu	$4, $4, 1
	.loc	2 1183
	bgt	$4, $8, $141
	mul	$14, $7, 4
	addu	$3, $2, $14
$140:
	.loc	2 1185
 #1184			{
 #1185			    sequence[l] = j;
	sw	$4, 0($3)
	.loc	2 1186
 #1186			}
	.loc	2 1186
	addu	$4, $4, 1
	addu	$8, $8, -1
	addu	$6, $6, -8
	addu	$7, $7, 1
	addu	$3, $3, 4
	.loc	2 1186
	ble	$4, $9, $140
	move	$9, $8
	addu	$10, $8, -1
$141:
	.loc	2 1187
 #1187		    }
	.loc	2 1187
	move	$4, $10
	.loc	2 1187
	bge	$8, 0, $137
$142:
	.loc	2 1188
 #1188		}
	move	$8, $0
	b	$145
$143:
	.loc	2 1197
 #1197		    for (i = 0, j = nrects - 1; i < nrects; i++, j--)
	move	$8, $0
	addu	$4, $22, -1
	.loc	2 1197
	ble	$22, 0, $145
	move	$3, $2
$144:
	.loc	2 1199
 #1198		    {
 #1199			sequence[i] = j;
	sw	$4, 0($3)
	.loc	2 1200
 #1200		    }
	.loc	2 1200
	addu	$8, $8, 1
	addu	$3, $3, 4
	addu	$4, $4, -1
	.loc	2 1200
	blt	$8, $22, $144
	move	$8, $0
	.loc	2 1201
 #1201		}
	.loc	2 1202
 #1202	    }
$145:
	lw	$19, 248($sp)
	.loc	2 1210
 #1210	    graphicsExposures = pGC->graphicsExposures;
	lw	$24, 220($sp)
	lw	$2, 16($24)
	sll	$15, $2, 20
	srl	$25, $15, 31
	sw	$25, 152($sp)
	.loc	2 1211
 #1211	    pGC->graphicsExposures = FALSE;
	and	$13, $2, -2049
	sw	$13, 16($24)
	.loc	2 1213
 #1212	    
 #1213	    dx = dstx - srcx;
	lw	$14, 224($sp)
	subu	$20, $11, $14
	.loc	2 1214
 #1214	    dy = dsty - srcy;
	lw	$15, 228($sp)
	subu	$21, $12, $15
	.loc	2 1222
 #1222	    if (plane != 0)
	beq	$19, 0, $146
	.loc	2 1224
 #1223	    {
 #1224		pixCopyProc = pBackingGC->ops->CopyPlane;
	lw	$25, 112($sp)
	lw	$13, 72($25)
	lw	$24, 16($13)
	sw	$24, 148($sp)
	.loc	2 1225
 #1225	    }
	b	$147
$146:
	.loc	2 1228
 #1226	    else
 #1227	    {
 #1228		pixCopyProc = pBackingGC->ops->CopyArea;
	lw	$14, 112($sp)
	lw	$15, 72($14)
	lw	$25, 12($15)
	sw	$25, 148($sp)
	.loc	2 1229
 #1229	    }
$147:
	.loc	2 1231
 #1230	    
 #1231	    for (i = 0; i < nrects; i++)
	.loc	2 1231
	ble	$22, 0, $151
	sw	$0, 104($sp)
	lw	$13, 192($sp)
	sw	$13, 100($sp)
	mul	$24, $22, 4
	sw	$24, 96($sp)
$148:
	.loc	2 1233
 #1232	    {
 #1233		pBox = boxes[sequence[i]].pBox;
	lw	$14, 100($sp)
	lw	$15, 0($14)
	mul	$25, $15, 8
	addu	$2, $30, $25
	lw	$16, 0($2)
	.loc	2 1241
 #1241		if (boxes[sequence[i]].source == pix)
	lw	$13, 4($2)
	bne	$13, 1, $149
	.loc	2 1246
 #1242		{
 #1243		    (void) (* copyProc) (pBackingDrawable, pWin, pGC,
 #1244				  pBox->x1, pBox->y1,
 #1245				  pBox->x2 - pBox->x1, pBox->y2 - pBox->y1,
 #1246				  pBox->x1 + dx, pBox->y1 + dy, plane);
	lw	$4, 124($sp)
	move	$5, $23
	lw	$6, 220($sp)
	lh	$17, 0($16)
	move	$7, $17
	lh	$18, 2($16)
	sw	$18, 16($sp)
	lh	$24, 4($16)
	subu	$14, $24, $17
	sw	$14, 20($sp)
	lh	$15, 6($16)
	subu	$25, $15, $18
	sw	$25, 24($sp)
	addu	$13, $17, $20
	sw	$13, 28($sp)
	addu	$24, $18, $21
	sw	$24, 32($sp)
	sw	$19, 36($sp)
	lw	$14, 252($sp)
	jal	$14
	lw	$4, 124($sp)
	.loc	2 1250
 #1247		    (void) (* pixCopyProc) (pBackingDrawable, pBackingDrawable, pBackingGC,
 #1248				     pBox->x1, pBox->y1,
 #1249				     pBox->x2 - pBox->x1, pBox->y2 - pBox->y1,
 #1250				     pBox->x1 + dx, pBox->y1 + dy, plane);
	move	$5, $4
	lw	$6, 112($sp)
	lh	$17, 0($16)
	move	$7, $17
	lh	$18, 2($16)
	sw	$18, 16($sp)
	lh	$15, 4($16)
	subu	$25, $15, $17
	sw	$25, 20($sp)
	lh	$13, 6($16)
	subu	$24, $13, $18
	sw	$24, 24($sp)
	addu	$14, $17, $20
	sw	$14, 28($sp)
	addu	$15, $18, $21
	sw	$15, 32($sp)
	sw	$19, 36($sp)
	lw	$25, 148($sp)
	jal	$25
	.loc	2 1251
 #1251		}
	b	$150
$149:
	.loc	2 1257
 #1252		else
 #1253		{
 #1254		    (void) (* pixCopyProc) (pWin, pBackingDrawable, pBackingGC,
 #1255				     pBox->x1, pBox->y1,
 #1256				     pBox->x2 - pBox->x1, pBox->y2 - pBox->y1,
 #1257				     pBox->x1 + dx, pBox->y1 + dy, plane);
	move	$4, $23
	lw	$5, 124($sp)
	lw	$6, 112($sp)
	lh	$17, 0($16)
	move	$7, $17
	lh	$18, 2($16)
	sw	$18, 16($sp)
	lh	$13, 4($16)
	subu	$24, $13, $17
	sw	$24, 20($sp)
	lh	$14, 6($16)
	subu	$15, $14, $18
	sw	$15, 24($sp)
	addu	$25, $17, $20
	sw	$25, 28($sp)
	addu	$13, $18, $21
	sw	$13, 32($sp)
	sw	$19, 36($sp)
	lw	$24, 148($sp)
	jal	$24
	.loc	2 1261
 #1258		    (void) (* copyProc) (pWin, pWin, pGC,
 #1259				  pBox->x1, pBox->y1,
 #1260				  pBox->x2 - pBox->x1, pBox->y2 - pBox->y1,
 #1261				  pBox->x1 + dx, pBox->y1 + dy, plane);
	move	$4, $23
	move	$5, $23
	lw	$6, 220($sp)
	lh	$17, 0($16)
	move	$7, $17
	lh	$18, 2($16)
	sw	$18, 16($sp)
	lh	$14, 4($16)
	subu	$15, $14, $17
	sw	$15, 20($sp)
	lh	$25, 6($16)
	subu	$13, $25, $18
	sw	$13, 24($sp)
	addu	$24, $17, $20
	sw	$24, 28($sp)
	addu	$14, $18, $21
	sw	$14, 32($sp)
	sw	$19, 36($sp)
	lw	$15, 252($sp)
	jal	$15
	.loc	2 1262
 #1262		}
$150:
	.loc	2 1263
 #1263	    }
	.loc	2 1263
	lw	$25, 104($sp)
	addu	$13, $25, 4
	sw	$13, 104($sp)
	lw	$24, 100($sp)
	addu	$14, $24, 4
	sw	$14, 100($sp)
	.loc	2 1263
	lw	$15, 96($sp)
	blt	$13, $15, $148
$151:
	.loc	2 1264
 #1264	    DEALLOCATE_LOCAL(sequence);
	lw	$4, 192($sp)
	jal	Xfree
	.loc	2 1265
 #1265	    DEALLOCATE_LOCAL(boxes);
	move	$4, $30
	jal	Xfree
	.loc	2 1267
 #1266	
 #1267	    pGC->graphicsExposures = graphicsExposures;
	lw	$25, 220($sp)
	lw	$2, 16($25)
	srl	$24, $2, 11
	lw	$14, 152($sp)
	xor	$13, $24, $14
	sll	$15, $13, 31
	srl	$24, $15, 20
	xor	$14, $24, $2
	sw	$14, 16($25)
	.loc	2 1268
 #1268	    if (graphicsExposures)
	lw	$13, 152($sp)
	beq	$13, 0, $154
	lw	$4, 212($sp)
	.loc	2 1276
 #1276		(* pGC->pScreen->Union) (pRgnExp, pRgnExp, pRgnObs);
	move	$5, $4
	lw	$6, 208($sp)
	lw	$15, 220($sp)
	lw	$24, 0($15)
	lw	$14, 328($24)
	jal	$14
	.loc	2 1277
 #1277		box.x1 = srcx;
	lw	$25, 224($sp)
	sh	$25, 200($sp)
	.loc	2 1278
 #1278		box.x2 = srcx + w;
	lw	$13, 232($sp)
	addu	$15, $25, $13
	sh	$15, 204($sp)
	.loc	2 1279
 #1279		box.y1 = srcy;
	lw	$24, 228($sp)
	sh	$24, 202($sp)
	.loc	2 1280
 #1280		box.y2 = srcy + h;
	lw	$14, 236($sp)
	addu	$25, $24, $14
	sh	$25, 206($sp)
	.loc	2 1281
 #1281		if ((* pGC->pScreen->RectIn) (pRgnExp, &box) == rgnIN)
	lw	$4, 212($sp)
	addu	$5, $sp, 200
	lw	$13, 220($sp)
	lw	$15, 0($13)
	lw	$24, 348($15)
	jal	$24
	bne	$2, 1, $152
	.loc	2 1282
 #1282		    (*pGC->pScreen->RegionEmpty) (pRgnExp);
	lw	$4, 212($sp)
	lw	$14, 220($sp)
	lw	$25, 0($14)
	lw	$13, 360($25)
	jal	$13
	b	$153
$152:
	.loc	2 1285
 #1283		else
 #1284		{
 #1285		    (* pGC->pScreen->Inverse) (pRgnExp, pRgnExp, &box);
	lw	$15, 212($sp)
	move	$4, $15
	move	$5, $15
	addu	$6, $sp, 200
	lw	$24, 220($sp)
	lw	$14, 0($24)
	lw	$25, 336($14)
	jal	$25
	.loc	2 1286
 #1286		    (* pGC->pScreen->TranslateRegion) (pRgnExp, dx, dy);
	lw	$4, 212($sp)
	move	$5, $20
	move	$6, $21
	lw	$13, 220($sp)
	lw	$15, 0($13)
	lw	$24, 344($15)
	jal	$24
	.loc	2 1287
 #1287		}
$153:
	.loc	2 1288
 #1288		*ppRgn = pRgnExp;
	lw	$14, 212($sp)
	lw	$25, 256($sp)
	sw	$14, 0($25)
	.loc	2 1289
 #1289	    }
	b	$155
$154:
	.loc	2 1292
 #1290	    else
 #1291	    {
 #1292		(*pGC->pScreen->RegionDestroy) (pRgnExp);
	lw	$4, 212($sp)
	lw	$13, 220($sp)
	lw	$15, 0($13)
	lw	$24, 316($15)
	jal	$24
	.loc	2 1293
 #1293	    }
$155:
	.loc	2 1294
 #1294	    (*pGC->pScreen->RegionDestroy) (pRgnObs);
	lw	$4, 208($sp)
	lw	$14, 220($sp)
	lw	$25, 0($14)
	lw	$13, 316($25)
	jal	$13
	.loc	2 1296
 #1295	
 #1296	    return (TRUE);
	li	$2, 1
	lw	$17, 84($sp)
	lw	$18, 80($sp)
	lw	$19, 76($sp)
	lw	$20, 64($sp)
	lw	$21, 60($sp)
	lw	$22, 72($sp)
	lw	$30, 68($sp)
	.endb	1148
$156:
	lw	$16, 44($sp)
	lw	$23, 48($sp)
	lw	$31, 52($sp)
	addu	$sp, 216
	j	$31
	.end	miBSDoCopy
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1326
 #1326	{
	.ent	$$865 2
$$865:
	.option	O2
	subu	$sp, 120
	sw	$31, 52($sp)
	sw	$16, 48($sp)
	.mask	0x80010000, -68
	.frame	$sp, 120, $31
	sw	$4, 120($sp)
	sw	$5, 124($sp)
	move	$16, $6
	sw	$7, 132($sp)
	.bgnb	1159
	.loc	2 1330
 #1327	    BoxPtr	pExtents;
 #1328	    long	dx, dy;
 #1329	    int		bsrcx, bsrcy, bw, bh, bdstx, bdsty;
 #1330	    RegionPtr	pixExposed = 0, winExposed = 0;
	sw	$0, 80($sp)
	.loc	2 1330
	sw	$0, 76($sp)
	.loc	2 1332
 #1331	
 #1332	    SETUP_BACKING(pDst, pGC);
	lw	$14, 124($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 72($sp)
	.loc	2 1332
	lw	$25, 76($16)
	lw	$14, $$853
	mul	$15, $14, 4
	addu	$24, $25, $15
	lw	$14, 0($24)
	sw	$14, 68($sp)
	.loc	2 1332
	lw	$25, 68($16)
	sw	$25, 64($sp)
	.loc	2 1332
	lw	$15, 68($sp)
	lw	$12, 0($15)
	.loc	2 1334
 #1333	
 #1334	    PROLOGUE(pGC);
	lw	$24, 16($15)
	sw	$24, 72($16)
	.loc	2 1334
	lw	$14, 68($sp)
	lw	$25, 20($14)
	sw	$25, 68($16)
	.loc	2 1334
	.loc	2 1338
 #1335	
 #1336	    if ((pSrc != pDst) ||
 #1337		(!miBSDoCopy((WindowPtr)pSrc, pGC, srcx, srcy, w, h, dstx, dsty,
 #1338			     (unsigned long) 0, pGC->ops->CopyArea, &winExposed)))
	lw	$15, 120($sp)
	lw	$24, 124($sp)
	bne	$15, $24, $157
	move	$4, $15
	move	$5, $16
	lw	$6, 132($sp)
	lw	$7, 136($sp)
	lw	$14, 140($sp)
	sw	$14, 16($sp)
	lw	$25, 144($sp)
	sw	$25, 20($sp)
	lw	$24, 148($sp)
	sw	$24, 24($sp)
	lw	$15, 152($sp)
	sw	$15, 28($sp)
	sw	$0, 32($sp)
	lw	$14, 72($16)
	lw	$25, 12($14)
	sw	$25, 36($sp)
	addu	$24, $sp, 76
	sw	$24, 40($sp)
	sw	$12, 60($sp)
	jal	$$1106
	lw	$12, 60($sp)
	bne	$2, 0, $164
$157:
	.loc	2 1346
 #1346		if (pGC->clientClipType != CT_PIXMAP)
	lw	$15, 16($16)
	sll	$14, $15, 18
	srl	$25, $14, 30
	beq	$25, 1, $162
	.loc	2 1354
 #1354			    (pBackingGC->clientClip);
	lw	$4, 56($12)
	sw	$12, 60($sp)
	lw	$24, 124($sp)
	lw	$15, 16($24)
	lw	$14, 364($15)
	jal	$14
	lw	$12, 60($sp)
	lw	$31, 148($sp)
	.loc	2 1355
 #1355		    bsrcx = srcx;
	lw	$25, 132($sp)
	move	$7, $25
	.loc	2 1356
 #1356		    bsrcy = srcy;
	lw	$13, 136($sp)
	.loc	2 1357
 #1357		    bw = w;
	lw	$24, 140($sp)
	move	$8, $24
	.loc	2 1358
 #1358		    bh = h;
	lw	$9, 144($sp)
	.loc	2 1359
 #1359		    bdstx = dstx;
	move	$10, $31
	.loc	2 1360
 #1360		    bdsty = dsty;
	lw	$11, 152($sp)
	.loc	2 1361
 #1361		    dx = pExtents->x1 - bdstx;
	lh	$15, 0($2)
	subu	$3, $15, $31
	.loc	2 1362
 #1362		    if (dx > 0)
	ble	$3, 0, $158
	.loc	2 1364
 #1363		    {
 #1364			bsrcx += dx;
	addu	$7, $25, $3
	.loc	2 1365
 #1365			bdstx += dx;
	addu	$10, $31, $3
	.loc	2 1366
 #1366			bw -= dx;
	subu	$8, $24, $3
	.loc	2 1367
 #1367		    }
$158:
	.loc	2 1368
 #1368		    dy = pExtents->y1 - bdsty;
	lh	$14, 2($2)
	lw	$15, 152($sp)
	subu	$3, $14, $15
	.loc	2 1369
 #1369		    if (dy > 0)
	ble	$3, 0, $159
	.loc	2 1371
 #1370		    {
 #1371			bsrcy += dy;
	lw	$25, 136($sp)
	addu	$13, $25, $3
	.loc	2 1372
 #1372			bdsty += dy;
	addu	$11, $15, $3
	.loc	2 1373
 #1373			bh -= dy;
	lw	$24, 144($sp)
	subu	$9, $24, $3
	.loc	2 1374
 #1374		    }
$159:
	.loc	2 1375
 #1375		    dx = (bdstx + bw) - pExtents->x2;
	addu	$14, $10, $8
	lh	$25, 4($2)
	subu	$3, $14, $25
	.loc	2 1376
 #1376		    if (dx > 0)
	ble	$3, 0, $160
	.loc	2 1377
 #1377			bw -= dx;
	subu	$8, $8, $3
$160:
	.loc	2 1378
 #1378		    dy = (bdsty + bh) - pExtents->y2;
	addu	$15, $11, $9
	lh	$24, 6($2)
	subu	$3, $15, $24
	.loc	2 1379
 #1379		    if (dy > 0)
	ble	$3, 0, $161
	.loc	2 1380
 #1380			bh -= dy;
	subu	$9, $9, $3
$161:
	.loc	2 1381
 #1381		    if (bw > 0 && bh > 0)
	ble	$8, 0, $163
	ble	$9, 0, $163
	.loc	2 1384
 #1382			pixExposed = (* pBackingGC->ops->CopyArea) (pSrc, 
 #1383				    pBackingDrawable, pBackingGC, 
 #1384				    bsrcx, bsrcy, bw, bh, bdstx, bdsty);
	lw	$4, 120($sp)
	lw	$5, 72($sp)
	move	$6, $12
	sw	$13, 16($sp)
	sw	$8, 20($sp)
	sw	$9, 24($sp)
	sw	$10, 28($sp)
	sw	$11, 32($sp)
	lw	$14, 72($12)
	lw	$25, 12($14)
	jal	$25
	sw	$2, 80($sp)
	lw	$31, 148($sp)
	.loc	2 1385
 #1385		}
	b	$163
$162:
	.loc	2 1389
 #1386		else
 #1387		    pixExposed = (* pBackingGC->ops->CopyArea) (pSrc, 
 #1388				    pBackingDrawable, pBackingGC,
 #1389				    srcx, srcy, w, h, dstx, dsty);
	lw	$4, 120($sp)
	lw	$5, 72($sp)
	move	$6, $12
	lw	$7, 132($sp)
	lw	$15, 136($sp)
	sw	$15, 16($sp)
	lw	$24, 140($sp)
	sw	$24, 20($sp)
	lw	$14, 144($sp)
	sw	$14, 24($sp)
	lw	$25, 148($sp)
	sw	$25, 28($sp)
	lw	$15, 152($sp)
	sw	$15, 32($sp)
	lw	$24, 72($12)
	lw	$14, 12($24)
	jal	$14
	sw	$2, 80($sp)
	lw	$31, 148($sp)
$163:
	.loc	2 1391
 #1390	
 #1391		winExposed = (* pGC->ops->CopyArea) (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty);
	lw	$4, 120($sp)
	lw	$5, 124($sp)
	move	$6, $16
	lw	$7, 132($sp)
	lw	$25, 136($sp)
	sw	$25, 16($sp)
	lw	$15, 140($sp)
	sw	$15, 20($sp)
	lw	$24, 144($sp)
	sw	$24, 24($sp)
	sw	$31, 28($sp)
	lw	$14, 152($sp)
	sw	$14, 32($sp)
	lw	$25, 72($16)
	lw	$15, 12($25)
	jal	$15
	sw	$2, 76($sp)
	.loc	2 1392
 #1392	    }
$164:
	.loc	2 1397
 #1393	
 #1394	    /*
 #1395	     * compute the composite graphics exposure region
 #1396	     */
 #1397	    if (winExposed)
	lw	$24, 76($sp)
	beq	$24, 0, $165
	.loc	2 1399
 #1398	    {
 #1399		if (pixExposed){
	lw	$14, 80($sp)
	beq	$14, 0, $166
	.loc	2 1400
 #1400		    (*pDst->pScreen->Union) (winExposed, winExposed, pixExposed);
	move	$4, $24
	move	$5, $24
	move	$6, $14
	lw	$25, 124($sp)
	lw	$15, 16($25)
	lw	$24, 328($15)
	jal	$24
	.loc	2 1401
 #1401		    (*pDst->pScreen->RegionDestroy) (pixExposed);
	lw	$4, 80($sp)
	lw	$14, 124($sp)
	lw	$25, 16($14)
	lw	$15, 316($25)
	jal	$15
	.loc	2 1402
 #1402		}
	.loc	2 1403
 #1403	    } else
	b	$166
$165:
	.loc	2 1404
 #1404		winExposed = pixExposed;
	lw	$24, 80($sp)
	sw	$24, 76($sp)
$166:
	.loc	2 1406
 #1405	
 #1406	    EPILOGUE (pGC);
	lw	$14, 72($16)
	lw	$25, 68($sp)
	sw	$14, 16($25)
	.loc	2 1406
	la	$15, $$883
	sw	$15, 72($16)
	.loc	2 1406
	lw	$24, 64($sp)
	sw	$24, 68($16)
	.loc	2 1406
	.loc	2 1408
 #1407	
 #1408	    return winExposed;
	lw	$2, 76($sp)
	.endb	1175
	lw	$16, 48($sp)
	lw	$31, 52($sp)
	addu	$sp, 120
	j	$31
	.end	miBSCopyArea
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1434
 #1434	{
	.ent	$$866 2
$$866:
	.option	O2
	subu	$sp, 120
	sw	$31, 52($sp)
	sw	$16, 48($sp)
	.mask	0x80010000, -68
	.frame	$sp, 120, $31
	sw	$4, 120($sp)
	sw	$5, 124($sp)
	move	$16, $6
	sw	$7, 132($sp)
	.bgnb	1188
	.loc	2 1438
 #1435	    BoxPtr	pExtents;
 #1436	    long	dx, dy;
 #1437	    int		bsrcx, bsrcy, bw, bh, bdstx, bdsty;
 #1438	    RegionPtr	winExposed = 0, pixExposed = 0;
	sw	$0, 80($sp)
	.loc	2 1438
	sw	$0, 76($sp)
	.loc	2 1439
 #1439	    SETUP_BACKING(pDst, pGC);
	lw	$14, 124($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 72($sp)
	.loc	2 1439
	lw	$25, 76($16)
	lw	$14, $$853
	mul	$15, $14, 4
	addu	$24, $25, $15
	lw	$14, 0($24)
	sw	$14, 68($sp)
	.loc	2 1439
	lw	$25, 68($16)
	sw	$25, 64($sp)
	.loc	2 1439
	lw	$15, 68($sp)
	lw	$12, 0($15)
	.loc	2 1441
 #1440	
 #1441	    PROLOGUE(pGC);
	lw	$24, 16($15)
	sw	$24, 72($16)
	.loc	2 1441
	lw	$14, 68($sp)
	lw	$25, 20($14)
	sw	$25, 68($16)
	.loc	2 1441
	.loc	2 1445
 #1442	
 #1443	    if ((pSrc != pDst) ||
 #1444		(!miBSDoCopy((WindowPtr)pSrc, pGC, srcx, srcy, w, h, dstx, dsty,
 #1445			     plane,  pGC->ops->CopyPlane, &winExposed)))
	lw	$15, 120($sp)
	lw	$24, 124($sp)
	bne	$15, $24, $167
	move	$4, $15
	move	$5, $16
	lw	$6, 132($sp)
	lw	$7, 136($sp)
	lw	$14, 140($sp)
	sw	$14, 16($sp)
	lw	$25, 144($sp)
	sw	$25, 20($sp)
	lw	$24, 148($sp)
	sw	$24, 24($sp)
	lw	$15, 152($sp)
	sw	$15, 28($sp)
	lw	$14, 156($sp)
	sw	$14, 32($sp)
	lw	$25, 72($16)
	lw	$24, 16($25)
	sw	$24, 36($sp)
	addu	$15, $sp, 80
	sw	$15, 40($sp)
	sw	$12, 60($sp)
	jal	$$1106
	lw	$12, 60($sp)
	bne	$2, 0, $174
$167:
	.loc	2 1453
 #1453		if (pGC->clientClipType != CT_PIXMAP)
	lw	$14, 16($16)
	sll	$25, $14, 18
	srl	$24, $25, 30
	beq	$24, 1, $172
	.loc	2 1460
 #1460		    pExtents = (*pDst->pScreen->RegionExtents) (pBackingGC->clientClip);
	lw	$4, 56($12)
	sw	$12, 60($sp)
	lw	$15, 124($sp)
	lw	$14, 16($15)
	lw	$25, 364($14)
	jal	$25
	lw	$12, 60($sp)
	lw	$31, 148($sp)
	.loc	2 1461
 #1461		    bsrcx = srcx;
	lw	$24, 132($sp)
	move	$7, $24
	.loc	2 1462
 #1462		    bsrcy = srcy;
	lw	$13, 136($sp)
	.loc	2 1463
 #1463		    bw = w;
	lw	$15, 140($sp)
	move	$8, $15
	.loc	2 1464
 #1464		    bh = h;
	lw	$9, 144($sp)
	.loc	2 1465
 #1465		    bdstx = dstx;
	move	$10, $31
	.loc	2 1466
 #1466		    bdsty = dsty;
	lw	$11, 152($sp)
	.loc	2 1467
 #1467		    dx = pExtents->x1 - bdstx;
	lh	$14, 0($2)
	subu	$3, $14, $31
	.loc	2 1468
 #1468		    if (dx > 0)
	ble	$3, 0, $168
	.loc	2 1470
 #1469		    {
 #1470			bsrcx += dx;
	addu	$7, $24, $3
	.loc	2 1471
 #1471			bdstx += dx;
	addu	$10, $31, $3
	.loc	2 1472
 #1472			bw -= dx;
	subu	$8, $15, $3
	.loc	2 1473
 #1473		    }
$168:
	.loc	2 1474
 #1474		    dy = pExtents->y1 - bdsty;
	lh	$25, 2($2)
	lw	$14, 152($sp)
	subu	$3, $25, $14
	.loc	2 1475
 #1475		    if (dy > 0)
	ble	$3, 0, $169
	.loc	2 1477
 #1476		    {
 #1477			bsrcy += dy;
	lw	$24, 136($sp)
	addu	$13, $24, $3
	.loc	2 1478
 #1478			bdsty += dy;
	addu	$11, $14, $3
	.loc	2 1479
 #1479			bh -= dy;
	lw	$15, 144($sp)
	subu	$9, $15, $3
	.loc	2 1480
 #1480		    }
$169:
	.loc	2 1481
 #1481		    dx = (bdstx + bw) - pExtents->x2;
	addu	$25, $10, $8
	lh	$24, 4($2)
	subu	$3, $25, $24
	.loc	2 1482
 #1482		    if (dx > 0)
	ble	$3, 0, $170
	.loc	2 1483
 #1483			bw -= dx;
	subu	$8, $8, $3
$170:
	.loc	2 1484
 #1484		    dy = (bdsty + bh) - pExtents->y2;
	addu	$14, $11, $9
	lh	$15, 6($2)
	subu	$3, $14, $15
	.loc	2 1485
 #1485		    if (dy > 0)
	ble	$3, 0, $171
	.loc	2 1486
 #1486			bh -= dy;
	subu	$9, $9, $3
$171:
	.loc	2 1487
 #1487		    if (bw > 0 && bh > 0)
	ble	$8, 0, $173
	ble	$9, 0, $173
	.loc	2 1491
 #1488			pixExposed = (* pBackingGC->ops->CopyPlane) (pSrc, 
 #1489					    pBackingDrawable,
 #1490					    pBackingGC, bsrcx, bsrcy, bw, bh,
 #1491					    bdstx, bdsty, plane);
	lw	$4, 120($sp)
	lw	$5, 72($sp)
	move	$6, $12
	sw	$13, 16($sp)
	sw	$8, 20($sp)
	sw	$9, 24($sp)
	sw	$10, 28($sp)
	sw	$11, 32($sp)
	lw	$25, 156($sp)
	sw	$25, 36($sp)
	lw	$24, 72($12)
	lw	$14, 16($24)
	jal	$14
	sw	$2, 76($sp)
	lw	$31, 148($sp)
	.loc	2 1492
 #1492		}
	b	$173
$172:
	.loc	2 1497
 #1493		else
 #1494		    pixExposed = (* pBackingGC->ops->CopyPlane) (pSrc, 
 #1495					    pBackingDrawable,
 #1496					    pBackingGC, srcx, srcy, w, h,
 #1497					    dstx, dsty, plane);
	lw	$4, 120($sp)
	lw	$5, 72($sp)
	move	$6, $12
	lw	$7, 132($sp)
	lw	$15, 136($sp)
	sw	$15, 16($sp)
	lw	$25, 140($sp)
	sw	$25, 20($sp)
	lw	$24, 144($sp)
	sw	$24, 24($sp)
	lw	$14, 148($sp)
	sw	$14, 28($sp)
	lw	$15, 152($sp)
	sw	$15, 32($sp)
	lw	$25, 156($sp)
	sw	$25, 36($sp)
	lw	$24, 72($12)
	lw	$14, 16($24)
	jal	$14
	sw	$2, 76($sp)
	lw	$31, 148($sp)
$173:
	.loc	2 1500
 #1498	
 #1499		winExposed = (* pGC->ops->CopyPlane) (pSrc, pDst, pGC, srcx, srcy, w, h,
 #1500				      dstx, dsty, plane);
	lw	$4, 120($sp)
	lw	$5, 124($sp)
	move	$6, $16
	lw	$7, 132($sp)
	lw	$15, 136($sp)
	sw	$15, 16($sp)
	lw	$25, 140($sp)
	sw	$25, 20($sp)
	lw	$24, 144($sp)
	sw	$24, 24($sp)
	sw	$31, 28($sp)
	lw	$14, 152($sp)
	sw	$14, 32($sp)
	lw	$15, 156($sp)
	sw	$15, 36($sp)
	lw	$25, 72($16)
	lw	$24, 16($25)
	jal	$24
	sw	$2, 80($sp)
	.loc	2 1502
 #1501		
 #1502	    }
$174:
	.loc	2 1507
 #1503	
 #1504	    /*
 #1505	     * compute the composite graphics exposure region
 #1506	     */
 #1507	    if (winExposed)
	lw	$14, 80($sp)
	beq	$14, 0, $175
	.loc	2 1509
 #1508	    {
 #1509		if (pixExposed)
	lw	$15, 76($sp)
	beq	$15, 0, $176
	.loc	2 1511
 #1510		{
 #1511		    (*pDst->pScreen->Union) (winExposed, winExposed, pixExposed);
	move	$4, $14
	move	$5, $14
	move	$6, $15
	lw	$25, 124($sp)
	lw	$24, 16($25)
	lw	$14, 328($24)
	jal	$14
	.loc	2 1512
 #1512		    (*pDst->pScreen->RegionDestroy) (pixExposed);
	lw	$4, 76($sp)
	lw	$15, 124($sp)
	lw	$25, 16($15)
	lw	$24, 316($25)
	jal	$24
	.loc	2 1513
 #1513		}
	.loc	2 1514
 #1514	    } else
	b	$176
$175:
	.loc	2 1515
 #1515		winExposed = pixExposed;
	lw	$14, 76($sp)
	sw	$14, 80($sp)
$176:
	.loc	2 1517
 #1516	
 #1517	    EPILOGUE (pGC);
	lw	$15, 72($16)
	lw	$25, 68($sp)
	sw	$15, 16($25)
	.loc	2 1517
	la	$24, $$883
	sw	$24, 72($16)
	.loc	2 1517
	lw	$14, 64($sp)
	sw	$14, 68($16)
	.loc	2 1517
	.loc	2 1519
 #1518	
 #1519	    return winExposed;
	lw	$2, 80($sp)
	.endb	1204
	lw	$16, 48($sp)
	lw	$31, 52($sp)
	addu	$sp, 120
	j	$31
	.end	miBSCopyPlane
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1541
 #1541	{
	.ent	$$867 2
$$867:
	.option	O2
	subu	$sp, 64
	sw	$31, 36($sp)
	sd	$16, 28($sp)
	.mask	0x80030000, -28
	.frame	$sp, 64, $31
	sw	$4, 64($sp)
	move	$16, $5
	sw	$6, 72($sp)
	sw	$7, 76($sp)
	.bgnb	1212
	.loc	2 1543
 #1542	    xPoint	  *pptCopy;
 #1543	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 64($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 56($sp)
	.loc	2 1543
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1543
	lw	$11, 68($16)
	sw	$11, 48($sp)
	.loc	2 1543
	lw	$12, 0($17)
	sw	$12, 44($sp)
	.loc	2 1545
 #1544	
 #1545	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1545
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1545
	.loc	2 1547
 #1546	
 #1547	    pptCopy = (xPoint *)ALLOCATE_LOCAL(npt*sizeof(xPoint));
	lw	$15, 76($sp)
	mul	$24, $15, 4
	sw	$24, 40($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1548
 #1548	    if (pptCopy)
	beq	$2, 0, $177
	.loc	2 1550
 #1549	    {
 #1550		bcopy((char *)pptInit,(char *)pptCopy,npt*sizeof(xPoint));
	lw	$4, 80($sp)
	lw	$6, 40($sp)
	sw	$5, 60($sp)
	jal	bcopy
	.loc	2 1552
 #1551	
 #1552		(* pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, pptInit);
	lw	$4, 64($sp)
	move	$5, $16
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$8, 80($sp)
	sw	$8, 16($sp)
	lw	$25, 72($16)
	lw	$9, 20($25)
	jal	$9
	lw	$5, 44($sp)
	.loc	2 1555
 #1553	
 #1554		(* pBackingGC->ops->PolyPoint) (pBackingDrawable,
 #1555					   pBackingGC, mode, npt, pptCopy);
	lw	$4, 56($sp)
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$10, 60($sp)
	sw	$10, 16($sp)
	lw	$11, 72($5)
	lw	$12, 20($11)
	jal	$12
	.loc	2 1557
 #1556	
 #1557		DEALLOCATE_LOCAL(pptCopy);
	lw	$4, 60($sp)
	jal	Xfree
	.loc	2 1558
 #1558	    }
$177:
	.loc	2 1560
 #1559	
 #1560	    EPILOGUE (pGC);
	lw	$13, 72($16)
	sw	$13, 16($17)
	.loc	2 1560
	la	$14, $$883
	sw	$14, 72($16)
	.loc	2 1560
	lw	$15, 48($sp)
	sw	$15, 68($16)
	.loc	2 1560
	.loc	2 1561
 #1561	}
	.endb	1218
	ld	$16, 28($sp)
	lw	$31, 36($sp)
	addu	$sp, 64
	j	$31
	.end	miBSPolyPoint
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1581
 #1581	{
	.ent	$$868 2
$$868:
	.option	O2
	subu	$sp, 64
	sw	$31, 36($sp)
	sd	$16, 28($sp)
	.mask	0x80030000, -28
	.frame	$sp, 64, $31
	sw	$4, 64($sp)
	move	$16, $5
	sw	$6, 72($sp)
	sw	$7, 76($sp)
	.bgnb	1226
	.loc	2 1583
 #1582	    DDXPointPtr	pptCopy;
 #1583	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 64($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 56($sp)
	.loc	2 1583
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1583
	lw	$11, 68($16)
	sw	$11, 48($sp)
	.loc	2 1583
	lw	$12, 0($17)
	sw	$12, 44($sp)
	.loc	2 1585
 #1584	
 #1585	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1585
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1585
	.loc	2 1587
 #1586	
 #1587	    pptCopy = (DDXPointPtr)ALLOCATE_LOCAL(npt*sizeof(DDXPointRec));
	lw	$15, 76($sp)
	mul	$24, $15, 4
	sw	$24, 40($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1588
 #1588	    if (pptCopy)
	beq	$2, 0, $178
	.loc	2 1590
 #1589	    {
 #1590		bcopy((char *)pptInit,(char *)pptCopy,npt*sizeof(DDXPointRec));
	lw	$4, 80($sp)
	lw	$6, 40($sp)
	sw	$5, 60($sp)
	jal	bcopy
	.loc	2 1592
 #1591	
 #1592		(* pGC->ops->Polylines)(pDrawable, pGC, mode, npt, pptInit);
	lw	$4, 64($sp)
	move	$5, $16
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$8, 80($sp)
	sw	$8, 16($sp)
	lw	$25, 72($16)
	lw	$9, 24($25)
	jal	$9
	lw	$5, 44($sp)
	.loc	2 1594
 #1593		(* pBackingGC->ops->Polylines)(pBackingDrawable,
 #1594					  pBackingGC, mode, npt, pptCopy);
	lw	$4, 56($sp)
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$10, 60($sp)
	sw	$10, 16($sp)
	lw	$11, 72($5)
	lw	$12, 24($11)
	jal	$12
	.loc	2 1595
 #1595		DEALLOCATE_LOCAL(pptCopy);
	lw	$4, 60($sp)
	jal	Xfree
	.loc	2 1596
 #1596	    }
$178:
	.loc	2 1598
 #1597	
 #1598	    EPILOGUE (pGC);
	lw	$13, 72($16)
	sw	$13, 16($17)
	.loc	2 1598
	la	$14, $$883
	sw	$14, 72($16)
	.loc	2 1598
	lw	$15, 48($sp)
	sw	$15, 68($16)
	.loc	2 1598
	.loc	2 1599
 #1599	}
	.endb	1232
	ld	$16, 28($sp)
	lw	$31, 36($sp)
	addu	$sp, 64
	j	$31
	.end	miBSPolylines
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1619
 #1619	{
	.ent	$$869 2
$$869:
	.option	O2
	subu	$sp, 56
	sw	$31, 28($sp)
	sd	$16, 20($sp)
	.mask	0x80030000, -28
	.frame	$sp, 56, $31
	sw	$4, 56($sp)
	move	$16, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1239
	.loc	2 1622
 #1620	    xSegment	*pSegsCopy;
 #1621	
 #1622	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 56($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 48($sp)
	.loc	2 1622
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1622
	lw	$11, 68($16)
	sw	$11, 40($sp)
	.loc	2 1622
	lw	$12, 0($17)
	sw	$12, 36($sp)
	.loc	2 1624
 #1623	
 #1624	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1624
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1624
	.loc	2 1626
 #1625	
 #1626	    pSegsCopy = (xSegment *)ALLOCATE_LOCAL(nseg*sizeof(xSegment));
	lw	$15, 64($sp)
	mul	$24, $15, 8
	sw	$24, 32($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1627
 #1627	    if (pSegsCopy)
	beq	$2, 0, $179
	.loc	2 1629
 #1628	    {
 #1629		bcopy((char *)pSegs,(char *)pSegsCopy,nseg*sizeof(xSegment));
	lw	$4, 68($sp)
	lw	$6, 32($sp)
	sw	$5, 52($sp)
	jal	bcopy
	.loc	2 1631
 #1630	
 #1631		(* pGC->ops->PolySegment)(pDrawable, pGC, nseg, pSegs);
	lw	$4, 56($sp)
	move	$5, $16
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$8, 72($16)
	lw	$25, 28($8)
	jal	$25
	lw	$5, 36($sp)
	.loc	2 1633
 #1632		(* pBackingGC->ops->PolySegment)(pBackingDrawable,
 #1633					    pBackingGC, nseg, pSegsCopy);
	lw	$4, 48($sp)
	lw	$6, 64($sp)
	lw	$7, 52($sp)
	lw	$9, 72($5)
	lw	$10, 28($9)
	jal	$10
	.loc	2 1635
 #1634	
 #1635		DEALLOCATE_LOCAL(pSegsCopy);
	lw	$4, 52($sp)
	jal	Xfree
	.loc	2 1636
 #1636	    }
$179:
	.loc	2 1638
 #1637	
 #1638	    EPILOGUE (pGC);
	lw	$11, 72($16)
	sw	$11, 16($17)
	.loc	2 1638
	la	$12, $$883
	sw	$12, 72($16)
	.loc	2 1638
	lw	$13, 40($sp)
	sw	$13, 68($16)
	.loc	2 1638
	.loc	2 1639
 #1639	}
	.endb	1245
	ld	$16, 20($sp)
	lw	$31, 28($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolySegment
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1659
 #1659	{
	.ent	$$870 2
$$870:
	.option	O2
	subu	$sp, 56
	sw	$31, 28($sp)
	sd	$16, 20($sp)
	.mask	0x80030000, -28
	.frame	$sp, 56, $31
	sw	$4, 56($sp)
	move	$16, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1252
	.loc	2 1661
 #1660	    xRectangle	*pRectsCopy;
 #1661	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 56($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 48($sp)
	.loc	2 1661
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1661
	lw	$11, 68($16)
	sw	$11, 40($sp)
	.loc	2 1661
	lw	$12, 0($17)
	sw	$12, 36($sp)
	.loc	2 1663
 #1662	
 #1663	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1663
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1663
	.loc	2 1665
 #1664	
 #1665	    pRectsCopy =(xRectangle *)ALLOCATE_LOCAL(nrects*sizeof(xRectangle));
	lw	$15, 64($sp)
	mul	$24, $15, 8
	sw	$24, 32($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1666
 #1666	    if (pRectsCopy)
	beq	$2, 0, $180
	.loc	2 1668
 #1667	    {
 #1668		bcopy((char *)pRects,(char *)pRectsCopy,nrects*sizeof(xRectangle));
	lw	$4, 68($sp)
	lw	$6, 32($sp)
	sw	$5, 52($sp)
	jal	bcopy
	.loc	2 1670
 #1669	
 #1670		(* pGC->ops->PolyRectangle)(pDrawable, pGC, nrects, pRects);
	lw	$4, 56($sp)
	move	$5, $16
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$8, 72($16)
	lw	$25, 32($8)
	jal	$25
	lw	$5, 36($sp)
	.loc	2 1672
 #1671		(* pBackingGC->ops->PolyRectangle)(pBackingDrawable,
 #1672					      pBackingGC, nrects, pRectsCopy);
	lw	$4, 48($sp)
	lw	$6, 64($sp)
	lw	$7, 52($sp)
	lw	$9, 72($5)
	lw	$10, 32($9)
	jal	$10
	.loc	2 1674
 #1673	
 #1674		DEALLOCATE_LOCAL(pRectsCopy);
	lw	$4, 52($sp)
	jal	Xfree
	.loc	2 1675
 #1675	    }
$180:
	.loc	2 1677
 #1676	
 #1677	    EPILOGUE (pGC);
	lw	$11, 72($16)
	sw	$11, 16($17)
	.loc	2 1677
	la	$12, $$883
	sw	$12, 72($16)
	.loc	2 1677
	lw	$13, 40($sp)
	sw	$13, 68($16)
	.loc	2 1677
	.loc	2 1678
 #1678	}
	.endb	1258
	ld	$16, 20($sp)
	lw	$31, 28($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolyRectangle
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1697
 #1697	{
	.ent	$$871 2
$$871:
	.option	O2
	subu	$sp, 56
	sw	$31, 28($sp)
	sd	$16, 20($sp)
	.mask	0x80030000, -28
	.frame	$sp, 56, $31
	sw	$4, 56($sp)
	move	$16, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1265
	.loc	2 1699
 #1698	    xArc  *pArcsCopy;
 #1699	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 56($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 48($sp)
	.loc	2 1699
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1699
	lw	$11, 68($16)
	sw	$11, 40($sp)
	.loc	2 1699
	lw	$12, 0($17)
	sw	$12, 36($sp)
	.loc	2 1701
 #1700	
 #1701	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1701
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1701
	.loc	2 1703
 #1702	
 #1703	    pArcsCopy = (xArc *)ALLOCATE_LOCAL(narcs*sizeof(xArc));
	lw	$15, 64($sp)
	mul	$24, $15, 12
	sw	$24, 32($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1704
 #1704	    if (pArcsCopy)
	beq	$2, 0, $181
	.loc	2 1706
 #1705	    {
 #1706		bcopy((char *)parcs,(char *)pArcsCopy,narcs*sizeof(xArc));
	lw	$4, 68($sp)
	lw	$6, 32($sp)
	sw	$5, 52($sp)
	jal	bcopy
	.loc	2 1708
 #1707	
 #1708		(* pGC->ops->PolyArc)(pDrawable, pGC, narcs, parcs);
	lw	$4, 56($sp)
	move	$5, $16
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$8, 72($16)
	lw	$25, 36($8)
	jal	$25
	lw	$5, 36($sp)
	.loc	2 1710
 #1709		(* pBackingGC->ops->PolyArc)(pBackingDrawable, pBackingGC,
 #1710					narcs, pArcsCopy);
	lw	$4, 48($sp)
	lw	$6, 64($sp)
	lw	$7, 52($sp)
	lw	$9, 72($5)
	lw	$10, 36($9)
	jal	$10
	.loc	2 1712
 #1711	
 #1712		DEALLOCATE_LOCAL(pArcsCopy);
	lw	$4, 52($sp)
	jal	Xfree
	.loc	2 1713
 #1713	    }
$181:
	.loc	2 1715
 #1714	
 #1715	    EPILOGUE (pGC);
	lw	$11, 72($16)
	sw	$11, 16($17)
	.loc	2 1715
	la	$12, $$883
	sw	$12, 72($16)
	.loc	2 1715
	lw	$13, 40($sp)
	sw	$13, 68($16)
	.loc	2 1715
	.loc	2 1716
 #1716	}
	.endb	1271
	ld	$16, 20($sp)
	lw	$31, 28($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolyArc
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1737
 #1737	{
	.ent	$$872 2
$$872:
	.option	O2
	subu	$sp, 64
	sw	$31, 36($sp)
	sd	$16, 28($sp)
	.mask	0x80030000, -28
	.frame	$sp, 64, $31
	sw	$4, 64($sp)
	move	$16, $5
	sw	$6, 72($sp)
	sw	$7, 76($sp)
	.bgnb	1280
	.loc	2 1739
 #1738	    DDXPointPtr	pPtsCopy;
 #1739	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 64($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 56($sp)
	.loc	2 1739
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1739
	lw	$11, 68($16)
	sw	$11, 48($sp)
	.loc	2 1739
	lw	$12, 0($17)
	sw	$12, 44($sp)
	.loc	2 1741
 #1740	
 #1741	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1741
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1741
	.loc	2 1743
 #1742	
 #1743	    pPtsCopy = (DDXPointPtr)ALLOCATE_LOCAL(count*sizeof(DDXPointRec));
	lw	$15, 80($sp)
	mul	$24, $15, 4
	sw	$24, 40($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1744
 #1744	    if (pPtsCopy)
	beq	$2, 0, $182
	.loc	2 1746
 #1745	    {
 #1746		bcopy((char *)pPts,(char *)pPtsCopy,count*sizeof(DDXPointRec));
	lw	$4, 84($sp)
	lw	$6, 40($sp)
	sw	$5, 60($sp)
	jal	bcopy
	.loc	2 1747
 #1747		(* pGC->ops->FillPolygon)(pDrawable, pGC, shape, mode, count, pPts);
	lw	$4, 64($sp)
	move	$5, $16
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$8, 80($sp)
	sw	$8, 16($sp)
	lw	$25, 84($sp)
	sw	$25, 20($sp)
	lw	$9, 72($16)
	lw	$10, 40($9)
	jal	$10
	lw	$5, 44($sp)
	.loc	2 1750
 #1748		(* pBackingGC->ops->FillPolygon)(pBackingDrawable,
 #1749					    pBackingGC, shape, mode,
 #1750					    count, pPtsCopy);
	lw	$4, 56($sp)
	lw	$6, 72($sp)
	lw	$7, 76($sp)
	lw	$11, 80($sp)
	sw	$11, 16($sp)
	lw	$12, 60($sp)
	sw	$12, 20($sp)
	lw	$13, 72($5)
	lw	$14, 40($13)
	jal	$14
	.loc	2 1752
 #1751	
 #1752		DEALLOCATE_LOCAL(pPtsCopy);
	lw	$4, 60($sp)
	jal	Xfree
	.loc	2 1753
 #1753	    }
$182:
	.loc	2 1755
 #1754	
 #1755	    EPILOGUE (pGC);
	lw	$15, 72($16)
	sw	$15, 16($17)
	.loc	2 1755
	la	$24, $$883
	sw	$24, 72($16)
	.loc	2 1755
	lw	$8, 48($sp)
	sw	$8, 68($16)
	.loc	2 1755
	.loc	2 1756
 #1756	}
	.endb	1286
	ld	$16, 28($sp)
	lw	$31, 36($sp)
	addu	$sp, 64
	j	$31
	.end	miBSFillPolygon
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1776
 #1776	{
	.ent	$$873 2
$$873:
	.option	O2
	subu	$sp, 56
	sw	$31, 28($sp)
	sd	$16, 20($sp)
	.mask	0x80030000, -28
	.frame	$sp, 56, $31
	sw	$4, 56($sp)
	move	$16, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1293
	.loc	2 1778
 #1777	    xRectangle	*pRectCopy;
 #1778	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 56($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 48($sp)
	.loc	2 1778
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1778
	lw	$11, 68($16)
	sw	$11, 40($sp)
	.loc	2 1778
	lw	$12, 0($17)
	sw	$12, 36($sp)
	.loc	2 1780
 #1779	
 #1780	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1780
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1780
	.loc	2 1783
 #1781	
 #1782	    pRectCopy =
 #1783		(xRectangle *)ALLOCATE_LOCAL(nrectFill*sizeof(xRectangle));
	lw	$15, 64($sp)
	mul	$24, $15, 8
	sw	$24, 32($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1784
 #1784	    if (pRectCopy)
	beq	$2, 0, $183
	.loc	2 1787
 #1785	    {
 #1786		bcopy((char *)prectInit,(char *)pRectCopy,
 #1787		      nrectFill*sizeof(xRectangle));
	lw	$4, 68($sp)
	lw	$6, 32($sp)
	sw	$5, 52($sp)
	jal	bcopy
	.loc	2 1789
 #1788	
 #1789		(* pGC->ops->PolyFillRect)(pDrawable, pGC, nrectFill, prectInit);
	lw	$4, 56($sp)
	move	$5, $16
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$8, 72($16)
	lw	$25, 44($8)
	jal	$25
	lw	$5, 36($sp)
	.loc	2 1791
 #1790		(* pBackingGC->ops->PolyFillRect)(pBackingDrawable,
 #1791					     pBackingGC, nrectFill, pRectCopy);
	lw	$4, 48($sp)
	lw	$6, 64($sp)
	lw	$7, 52($sp)
	lw	$9, 72($5)
	lw	$10, 44($9)
	jal	$10
	.loc	2 1793
 #1792	
 #1793		DEALLOCATE_LOCAL(pRectCopy);
	lw	$4, 52($sp)
	jal	Xfree
	.loc	2 1794
 #1794	    }
$183:
	.loc	2 1796
 #1795	
 #1796	    EPILOGUE (pGC);
	lw	$11, 72($16)
	sw	$11, 16($17)
	.loc	2 1796
	la	$12, $$883
	sw	$12, 72($16)
	.loc	2 1796
	lw	$13, 40($sp)
	sw	$13, 68($16)
	.loc	2 1796
	.loc	2 1797
 #1797	}
	.endb	1299
	ld	$16, 20($sp)
	lw	$31, 28($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolyFillRect
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1817
 #1817	{
	.ent	$$874 2
$$874:
	.option	O2
	subu	$sp, 56
	sw	$31, 28($sp)
	sd	$16, 20($sp)
	.mask	0x80030000, -28
	.frame	$sp, 56, $31
	sw	$4, 56($sp)
	move	$16, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1306
	.loc	2 1819
 #1818	    xArc  *pArcsCopy;
 #1819	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 56($sp)
	lw	$15, 116($14)
	lw	$24, 0($15)
	sw	$24, 48($sp)
	.loc	2 1819
	lw	$25, 76($16)
	lw	$8, $$853
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$17, 0($10)
	.loc	2 1819
	lw	$11, 68($16)
	sw	$11, 40($sp)
	.loc	2 1819
	lw	$12, 0($17)
	sw	$12, 36($sp)
	.loc	2 1821
 #1820	
 #1821	    PROLOGUE(pGC);
	lw	$13, 16($17)
	sw	$13, 72($16)
	.loc	2 1821
	lw	$14, 20($17)
	sw	$14, 68($16)
	.loc	2 1821
	.loc	2 1823
 #1822	
 #1823	    pArcsCopy = (xArc *)ALLOCATE_LOCAL(narcs*sizeof(xArc));
	lw	$15, 64($sp)
	mul	$24, $15, 12
	sw	$24, 32($sp)
	move	$4, $24
	jal	Xalloc
	move	$5, $2
	.loc	2 1824
 #1824	    if (pArcsCopy)
	beq	$2, 0, $184
	.loc	2 1826
 #1825	    {
 #1826		bcopy((char *)parcs,(char *)pArcsCopy,narcs*sizeof(xArc));
	lw	$4, 68($sp)
	lw	$6, 32($sp)
	sw	$5, 52($sp)
	jal	bcopy
	.loc	2 1827
 #1827		(* pGC->ops->PolyFillArc)(pDrawable, pGC, narcs, parcs);
	lw	$4, 56($sp)
	move	$5, $16
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$8, 72($16)
	lw	$25, 48($8)
	jal	$25
	lw	$5, 36($sp)
	.loc	2 1829
 #1828		(* pBackingGC->ops->PolyFillArc)(pBackingDrawable,
 #1829					    pBackingGC, narcs, pArcsCopy);
	lw	$4, 48($sp)
	lw	$6, 64($sp)
	lw	$7, 52($sp)
	lw	$9, 72($5)
	lw	$10, 48($9)
	jal	$10
	.loc	2 1830
 #1830		DEALLOCATE_LOCAL(pArcsCopy);
	lw	$4, 52($sp)
	jal	Xfree
	.loc	2 1831
 #1831	    }
$184:
	.loc	2 1833
 #1832	
 #1833	    EPILOGUE (pGC);
	lw	$11, 72($16)
	sw	$11, 16($17)
	.loc	2 1833
	la	$12, $$883
	sw	$12, 72($16)
	.loc	2 1833
	lw	$13, 40($sp)
	sw	$13, 68($16)
	.loc	2 1833
	.loc	2 1834
 #1834	}
	.endb	1312
	ld	$16, 20($sp)
	lw	$31, 28($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolyFillArc
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1855
 #1855	{
	.ent	$$875 2
$$875:
	.option	O2
	subu	$sp, 56
	sw	$31, 28($sp)
	.mask	0x80000000, -28
	.frame	$sp, 56, $31
	move	$3, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1321
	.loc	2 1857
 #1856	    int	    result;
 #1857	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 116($4)
	lw	$15, 0($14)
	sw	$15, 48($sp)
	.loc	2 1857
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$10, $25, 4
	addu	$11, $24, $10
	lw	$8, 0($11)
	.loc	2 1857
	lw	$12, 68($3)
	sw	$12, 40($sp)
	.loc	2 1857
	lw	$9, 0($8)
	.loc	2 1859
 #1858	
 #1859	    PROLOGUE(pGC);
	lw	$13, 16($8)
	sw	$13, 72($3)
	.loc	2 1859
	lw	$14, 20($8)
	sw	$14, 68($3)
	.loc	2 1859
	.loc	2 1861
 #1860	
 #1861	    result = (* pGC->ops->PolyText8)(pDrawable, pGC, x, y, count, chars);
	move	$5, $3
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$15, 72($sp)
	sw	$15, 16($sp)
	lw	$25, 76($sp)
	sw	$25, 20($sp)
	sw	$3, 60($sp)
	sw	$8, 44($sp)
	sw	$9, 36($sp)
	lw	$24, 72($3)
	lw	$10, 52($24)
	jal	$10
	lw	$3, 60($sp)
	lw	$8, 44($sp)
	lw	$9, 36($sp)
	sw	$2, 52($sp)
	.loc	2 1863
 #1862	    (* pBackingGC->ops->PolyText8)(pBackingDrawable,
 #1863				      pBackingGC, x, y, count, chars);
	lw	$4, 48($sp)
	move	$5, $9
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$11, 72($sp)
	sw	$11, 16($sp)
	lw	$12, 76($sp)
	sw	$12, 20($sp)
	lw	$13, 72($9)
	lw	$14, 52($13)
	jal	$14
	lw	$3, 60($sp)
	lw	$8, 44($sp)
	.loc	2 1865
 #1864	
 #1865	    EPILOGUE (pGC);
	lw	$15, 72($3)
	sw	$15, 16($8)
	.loc	2 1865
	la	$25, $$883
	sw	$25, 72($3)
	.loc	2 1865
	lw	$24, 40($sp)
	sw	$24, 68($3)
	.loc	2 1865
	.loc	2 1866
 #1866	    return result;
	lw	$2, 52($sp)
	.endb	1327
	lw	$31, 28($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolyText8
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1887
 #1887	{
	.ent	$$876 2
$$876:
	.option	O2
	subu	$sp, 56
	sw	$31, 28($sp)
	.mask	0x80000000, -28
	.frame	$sp, 56, $31
	move	$3, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1336
	.loc	2 1889
 #1888	    int	result;
 #1889	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 116($4)
	lw	$15, 0($14)
	sw	$15, 48($sp)
	.loc	2 1889
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$10, $25, 4
	addu	$11, $24, $10
	lw	$8, 0($11)
	.loc	2 1889
	lw	$12, 68($3)
	sw	$12, 40($sp)
	.loc	2 1889
	lw	$9, 0($8)
	.loc	2 1891
 #1890	
 #1891	    PROLOGUE(pGC);
	lw	$13, 16($8)
	sw	$13, 72($3)
	.loc	2 1891
	lw	$14, 20($8)
	sw	$14, 68($3)
	.loc	2 1891
	.loc	2 1893
 #1892	
 #1893	    result = (* pGC->ops->PolyText16)(pDrawable, pGC, x, y, count, chars);
	move	$5, $3
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$15, 72($sp)
	sw	$15, 16($sp)
	lw	$25, 76($sp)
	sw	$25, 20($sp)
	sw	$3, 60($sp)
	sw	$8, 44($sp)
	sw	$9, 36($sp)
	lw	$24, 72($3)
	lw	$10, 56($24)
	jal	$10
	lw	$3, 60($sp)
	lw	$8, 44($sp)
	lw	$9, 36($sp)
	sw	$2, 52($sp)
	.loc	2 1895
 #1894	    (* pBackingGC->ops->PolyText16)(pBackingDrawable,
 #1895				       pBackingGC, x, y, count, chars);
	lw	$4, 48($sp)
	move	$5, $9
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$11, 72($sp)
	sw	$11, 16($sp)
	lw	$12, 76($sp)
	sw	$12, 20($sp)
	lw	$13, 72($9)
	lw	$14, 56($13)
	jal	$14
	lw	$3, 60($sp)
	lw	$8, 44($sp)
	.loc	2 1897
 #1896	
 #1897	    EPILOGUE (pGC);
	lw	$15, 72($3)
	sw	$15, 16($8)
	.loc	2 1897
	la	$25, $$883
	sw	$25, 72($3)
	.loc	2 1897
	lw	$24, 40($sp)
	sw	$24, 68($3)
	.loc	2 1897
	.loc	2 1899
 #1898	
 #1899	    return result;
	lw	$2, 52($sp)
	.endb	1342
	lw	$31, 28($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolyText16
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1920
 #1920	{
	.ent	$$877 2
$$877:
	.option	O2
	subu	$sp, 48
	sw	$31, 28($sp)
	.mask	0x80000000, -20
	.frame	$sp, 48, $31
	move	$3, $5
	sw	$6, 56($sp)
	sw	$7, 60($sp)
	.bgnb	1351
	.loc	2 1921
 #1921	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 116($4)
	lw	$15, 0($14)
	sw	$15, 44($sp)
	.loc	2 1921
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$9, $25, 4
	addu	$10, $24, $9
	lw	$2, 0($10)
	.loc	2 1921
	lw	$11, 68($3)
	sw	$11, 36($sp)
	.loc	2 1921
	lw	$8, 0($2)
	.loc	2 1922
 #1922	    PROLOGUE(pGC);
	lw	$12, 16($2)
	sw	$12, 72($3)
	.loc	2 1922
	lw	$13, 20($2)
	sw	$13, 68($3)
	.loc	2 1922
	.loc	2 1924
 #1923	
 #1924	    (* pGC->ops->ImageText8)(pDrawable, pGC, x, y, count, chars);
	move	$5, $3
	lw	$6, 56($sp)
	lw	$7, 60($sp)
	lw	$14, 64($sp)
	sw	$14, 16($sp)
	lw	$15, 68($sp)
	sw	$15, 20($sp)
	sw	$2, 40($sp)
	sw	$3, 52($sp)
	sw	$8, 32($sp)
	lw	$25, 72($3)
	lw	$24, 60($25)
	jal	$24
	lw	$2, 40($sp)
	lw	$3, 52($sp)
	lw	$8, 32($sp)
	.loc	2 1926
 #1925	    (* pBackingGC->ops->ImageText8)(pBackingDrawable,
 #1926				       pBackingGC, x, y, count, chars);
	lw	$4, 44($sp)
	move	$5, $8
	lw	$6, 56($sp)
	lw	$7, 60($sp)
	lw	$9, 64($sp)
	sw	$9, 16($sp)
	lw	$10, 68($sp)
	sw	$10, 20($sp)
	lw	$11, 72($8)
	lw	$12, 60($11)
	jal	$12
	lw	$2, 40($sp)
	lw	$3, 52($sp)
	.loc	2 1928
 #1927	
 #1928	    EPILOGUE (pGC);
	lw	$13, 72($3)
	sw	$13, 16($2)
	.loc	2 1928
	la	$14, $$883
	sw	$14, 72($3)
	.loc	2 1928
	lw	$15, 36($sp)
	sw	$15, 68($3)
	.loc	2 1928
	.loc	2 1929
 #1929	}
	.endb	1356
	lw	$31, 28($sp)
	addu	$sp, 48
	j	$31
	.end	miBSImageText8
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1949
 #1949	{
	.ent	$$878 2
$$878:
	.option	O2
	subu	$sp, 48
	sw	$31, 28($sp)
	.mask	0x80000000, -20
	.frame	$sp, 48, $31
	move	$3, $5
	sw	$6, 56($sp)
	sw	$7, 60($sp)
	.bgnb	1365
	.loc	2 1950
 #1950	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 116($4)
	lw	$15, 0($14)
	sw	$15, 44($sp)
	.loc	2 1950
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$9, $25, 4
	addu	$10, $24, $9
	lw	$2, 0($10)
	.loc	2 1950
	lw	$11, 68($3)
	sw	$11, 36($sp)
	.loc	2 1950
	lw	$8, 0($2)
	.loc	2 1951
 #1951	    PROLOGUE(pGC);
	lw	$12, 16($2)
	sw	$12, 72($3)
	.loc	2 1951
	lw	$13, 20($2)
	sw	$13, 68($3)
	.loc	2 1951
	.loc	2 1953
 #1952	
 #1953	    (* pGC->ops->ImageText16)(pDrawable, pGC, x, y, count, chars);
	move	$5, $3
	lw	$6, 56($sp)
	lw	$7, 60($sp)
	lw	$14, 64($sp)
	sw	$14, 16($sp)
	lw	$15, 68($sp)
	sw	$15, 20($sp)
	sw	$2, 40($sp)
	sw	$3, 52($sp)
	sw	$8, 32($sp)
	lw	$25, 72($3)
	lw	$24, 64($25)
	jal	$24
	lw	$2, 40($sp)
	lw	$3, 52($sp)
	lw	$8, 32($sp)
	.loc	2 1955
 #1954	    (* pBackingGC->ops->ImageText16)(pBackingDrawable,
 #1955					pBackingGC, x, y, count, chars);
	lw	$4, 44($sp)
	move	$5, $8
	lw	$6, 56($sp)
	lw	$7, 60($sp)
	lw	$9, 64($sp)
	sw	$9, 16($sp)
	lw	$10, 68($sp)
	sw	$10, 20($sp)
	lw	$11, 72($8)
	lw	$12, 64($11)
	jal	$12
	lw	$2, 40($sp)
	lw	$3, 52($sp)
	.loc	2 1957
 #1956	
 #1957	    EPILOGUE (pGC);
	lw	$13, 72($3)
	sw	$13, 16($2)
	.loc	2 1957
	la	$14, $$883
	sw	$14, 72($3)
	.loc	2 1957
	lw	$15, 36($sp)
	sw	$15, 68($3)
	.loc	2 1957
	.loc	2 1958
 #1958	}
	.endb	1370
	lw	$31, 28($sp)
	addu	$sp, 48
	j	$31
	.end	miBSImageText16
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 1979
 #1979	{
	.ent	$$879 2
$$879:
	.option	O2
	subu	$sp, 56
	sw	$31, 36($sp)
	.mask	0x80000000, -20
	.frame	$sp, 56, $31
	move	$3, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1380
	.loc	2 1980
 #1980	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 116($4)
	lw	$15, 0($14)
	sw	$15, 52($sp)
	.loc	2 1980
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$9, $25, 4
	addu	$10, $24, $9
	lw	$2, 0($10)
	.loc	2 1980
	lw	$11, 68($3)
	sw	$11, 44($sp)
	.loc	2 1980
	lw	$8, 0($2)
	.loc	2 1981
 #1981	    PROLOGUE(pGC);
	lw	$12, 16($2)
	sw	$12, 72($3)
	.loc	2 1981
	lw	$13, 20($2)
	sw	$13, 68($3)
	.loc	2 1981
	.loc	2 1984
 #1982	
 #1983	    (* pGC->ops->ImageGlyphBlt)(pDrawable, pGC, x, y, nglyph, ppci,
 #1984				     pglyphBase);
	move	$5, $3
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$14, 72($sp)
	sw	$14, 16($sp)
	lw	$15, 76($sp)
	sw	$15, 20($sp)
	lw	$25, 80($sp)
	sw	$25, 24($sp)
	sw	$2, 48($sp)
	sw	$3, 60($sp)
	sw	$8, 40($sp)
	lw	$24, 72($3)
	lw	$9, 68($24)
	jal	$9
	lw	$2, 48($sp)
	lw	$3, 60($sp)
	lw	$8, 40($sp)
	.loc	2 1987
 #1985	    (* pBackingGC->ops->ImageGlyphBlt)(pBackingDrawable,
 #1986					  pBackingGC, x, y, nglyph, ppci,
 #1987					  pglyphBase);
	lw	$4, 52($sp)
	move	$5, $8
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$10, 72($sp)
	sw	$10, 16($sp)
	lw	$11, 76($sp)
	sw	$11, 20($sp)
	lw	$12, 80($sp)
	sw	$12, 24($sp)
	lw	$13, 72($8)
	lw	$14, 68($13)
	jal	$14
	lw	$2, 48($sp)
	lw	$3, 60($sp)
	.loc	2 1989
 #1988	
 #1989	    EPILOGUE (pGC);
	lw	$15, 72($3)
	sw	$15, 16($2)
	.loc	2 1989
	la	$25, $$883
	sw	$25, 72($3)
	.loc	2 1989
	lw	$24, 44($sp)
	sw	$24, 68($3)
	.loc	2 1989
	.loc	2 1990
 #1990	}
	.endb	1385
	lw	$31, 36($sp)
	addu	$sp, 56
	j	$31
	.end	miBSImageGlyphBlt
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2011
 #2011	{
	.ent	$$880 2
$$880:
	.option	O2
	subu	$sp, 56
	sw	$31, 36($sp)
	.mask	0x80000000, -20
	.frame	$sp, 56, $31
	move	$3, $5
	sw	$6, 64($sp)
	sw	$7, 68($sp)
	.bgnb	1395
	.loc	2 2012
 #2012	    SETUP_BACKING (pDrawable, pGC);
	lw	$14, 116($4)
	lw	$15, 0($14)
	sw	$15, 52($sp)
	.loc	2 2012
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$9, $25, 4
	addu	$10, $24, $9
	lw	$2, 0($10)
	.loc	2 2012
	lw	$11, 68($3)
	sw	$11, 44($sp)
	.loc	2 2012
	lw	$8, 0($2)
	.loc	2 2013
 #2013	    PROLOGUE(pGC);
	lw	$12, 16($2)
	sw	$12, 72($3)
	.loc	2 2013
	lw	$13, 20($2)
	sw	$13, 68($3)
	.loc	2 2013
	.loc	2 2016
 #2014	
 #2015	    (* pGC->ops->PolyGlyphBlt)(pDrawable, pGC, x, y, nglyph,
 #2016				    ppci, pglyphBase);
	move	$5, $3
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$14, 72($sp)
	sw	$14, 16($sp)
	lw	$15, 76($sp)
	sw	$15, 20($sp)
	lw	$25, 80($sp)
	sw	$25, 24($sp)
	sw	$2, 48($sp)
	sw	$3, 60($sp)
	sw	$8, 40($sp)
	lw	$24, 72($3)
	lw	$9, 72($24)
	jal	$9
	lw	$2, 48($sp)
	lw	$3, 60($sp)
	lw	$8, 40($sp)
	.loc	2 2019
 #2017	    (* pBackingGC->ops->PolyGlyphBlt)(pBackingDrawable,
 #2018					 pBackingGC, x, y, nglyph, ppci,
 #2019					 pglyphBase);
	lw	$4, 52($sp)
	move	$5, $8
	lw	$6, 64($sp)
	lw	$7, 68($sp)
	lw	$10, 72($sp)
	sw	$10, 16($sp)
	lw	$11, 76($sp)
	sw	$11, 20($sp)
	lw	$12, 80($sp)
	sw	$12, 24($sp)
	lw	$13, 72($8)
	lw	$14, 72($13)
	jal	$14
	lw	$2, 48($sp)
	lw	$3, 60($sp)
	.loc	2 2020
 #2020	    EPILOGUE (pGC);
	lw	$15, 72($3)
	sw	$15, 16($2)
	.loc	2 2020
	la	$25, $$883
	sw	$25, 72($3)
	.loc	2 2020
	lw	$24, 44($sp)
	sw	$24, 68($3)
	.loc	2 2020
	.loc	2 2021
 #2021	}
	.endb	1400
	lw	$31, 36($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPolyGlyphBlt
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2040
 #2040	{
	.ent	$$881 2
$$881:
	.option	O2
	subu	$sp, 56
	sw	$31, 36($sp)
	.mask	0x80000000, -20
	.frame	$sp, 56, $31
	move	$3, $4
	sw	$5, 60($sp)
	sw	$7, 68($sp)
	.bgnb	1410
	.loc	2 2041
 #2041	    SETUP_BACKING (pDst, pGC);
	lw	$14, 116($6)
	lw	$15, 0($14)
	sw	$15, 52($sp)
	.loc	2 2041
	lw	$24, 76($3)
	lw	$25, $$853
	mul	$9, $25, 4
	addu	$10, $24, $9
	lw	$2, 0($10)
	.loc	2 2041
	lw	$11, 68($3)
	sw	$11, 44($sp)
	.loc	2 2041
	lw	$8, 0($2)
	.loc	2 2042
 #2042	    PROLOGUE(pGC);
	lw	$12, 16($2)
	sw	$12, 72($3)
	.loc	2 2042
	lw	$13, 20($2)
	sw	$13, 68($3)
	.loc	2 2042
	.loc	2 2044
 #2043	
 #2044	    (* pGC->ops->PushPixels)(pGC, pBitMap, pDst, w, h, x, y);
	move	$4, $3
	lw	$5, 60($sp)
	lw	$7, 68($sp)
	lw	$14, 72($sp)
	sw	$14, 16($sp)
	lw	$15, 76($sp)
	sw	$15, 20($sp)
	lw	$25, 80($sp)
	sw	$25, 24($sp)
	sw	$2, 48($sp)
	sw	$3, 56($sp)
	sw	$8, 40($sp)
	lw	$24, 72($3)
	lw	$9, 76($24)
	jal	$9
	lw	$2, 48($sp)
	lw	$3, 56($sp)
	lw	$8, 40($sp)
	.loc	2 2047
 #2045	    (* pBackingGC->ops->PushPixels)(pBackingGC, pBitMap,
 #2046				       pBackingDrawable, w, h,
 #2047				       x, y);
	move	$4, $8
	lw	$5, 60($sp)
	lw	$6, 52($sp)
	lw	$7, 68($sp)
	lw	$10, 72($sp)
	sw	$10, 16($sp)
	lw	$11, 76($sp)
	sw	$11, 20($sp)
	lw	$12, 80($sp)
	sw	$12, 24($sp)
	lw	$13, 72($8)
	lw	$14, 76($13)
	jal	$14
	lw	$2, 48($sp)
	lw	$3, 56($sp)
	.loc	2 2049
 #2048	
 #2049	    EPILOGUE (pGC);
	lw	$15, 72($3)
	sw	$15, 16($2)
	.loc	2 2049
	la	$25, $$883
	sw	$25, 72($3)
	.loc	2 2049
	lw	$24, 44($sp)
	sw	$24, 68($3)
	.loc	2 2049
	.loc	2 2050
 #2050	}
	.endb	1415
	lw	$31, 36($sp)
	addu	$sp, 56
	j	$31
	.end	miBSPushPixels
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2064
 #2064	{
	.ent	$$882 2
$$882:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1418
	.loc	2 2065
 #2065	    FatalError("miBSLineHelper called\n");
	la	$4, $$1419
	jal	FatalError
	.loc	2 2066
 #2066	}
	.endb	1420
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSLineHelper
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2094
 #2094	{
	.ent	$$851 2
$$851:
	.option	O2
	subu	$sp, 112
	sw	$31, 20($sp)
	.mask	0x80000000, -92
	.frame	$sp, 112, $31
	move	$8, $4
	move	$3, $5
	.bgnb	1429
	.loc	2 2111
 #2111	    pBackingStore = (miBSWindowPtr)pWin->backStorage;
	lw	$2, 116($8)
	.loc	2 2112
 #2112	    pScreen = pWin->drawable.pScreen;
	lw	$14, 16($8)
	sw	$14, 96($sp)
	.loc	2 2114
 #2113	
 #2114	    if (pBackingStore->status == StatusNoPixmap)
	lb	$15, 17($2)
	bne	$15, 1, $185
	.loc	2 2115
 #2115		return NullRegion;
	move	$2, $0
	b	$216
$185:
	.loc	2 2117
 #2116	    
 #2117	    if (w == 0)
	bne	$7, 0, $186
	.loc	2 2118
 #2118		w = (int) pWin->drawable.width - x;
	lhu	$24, 12($8)
	subu	$7, $24, $3
$186:
	.loc	2 2119
 #2119	    if (h == 0)
	lw	$25, 128($sp)
	bne	$25, 0, $187
	.loc	2 2120
 #2120		h = (int) pWin->drawable.height - y;
	lhu	$10, 14($8)
	subu	$11, $10, $6
	sw	$11, 128($sp)
$187:
	.loc	2 2122
 #2121	
 #2122	    box.x1 = x;
	sh	$3, 48($sp)
	.loc	2 2123
 #2123	    box.y1 = y;
	sh	$6, 50($sp)
	.loc	2 2124
 #2124	    box.x2 = x + w;
	addu	$12, $3, $7
	sh	$12, 52($sp)
	.loc	2 2125
 #2125	    box.y2 = y + h;
	lw	$13, 128($sp)
	addu	$14, $6, $13
	sh	$14, 54($sp)
	.loc	2 2126
 #2126	    pRgn = (*pWin->drawable.pScreen->RegionCreate)(&box, 1);
	addu	$4, $sp, 48
	li	$5, 1
	sw	$2, 100($sp)
	sw	$3, 116($sp)
	sw	$6, 120($sp)
	sw	$7, 124($sp)
	sw	$8, 112($sp)
	lw	$15, 16($8)
	lw	$24, 304($15)
	jal	$24
	move	$4, $2
	.loc	2 2127
 #2127	    if (!pRgn)
	bne	$2, 0, $188
	.loc	2 2128
 #2128		return NullRegion;
	move	$2, $0
	b	$216
$188:
	.loc	2 2129
 #2129	    (* pScreen->Intersect) (pRgn, pRgn, &pBackingStore->SavedRegion);
	move	$5, $4
	lw	$6, 100($sp)
	addu	$6, $6, 4
	sw	$4, 108($sp)
	lw	$25, 96($sp)
	lw	$10, 324($25)
	jal	$10
	.loc	2 2131
 #2130	
 #2131	    if ((* pScreen->RegionNotEmpty) (pRgn))
	lw	$4, 108($sp)
	lw	$11, 96($sp)
	lw	$12, 356($11)
	jal	$12
	beq	$2, 0, $214
	.loc	2 2140
 #2140	 	    h == pWin->drawable.height)
	lw	$13, 116($sp)
	beq	$13, 0, $189
	lw	$7, 112($sp)
	lw	$6, 100($sp)
	b	$194
$189:
	lw	$14, 120($sp)
	beq	$14, 0, $190
	lw	$7, 112($sp)
	lw	$6, 100($sp)
	b	$194
$190:
	lw	$7, 112($sp)
	lw	$15, 124($sp)
	lhu	$24, 12($7)
	beq	$15, $24, $191
	lw	$6, 100($sp)
	b	$194
$191:
	lw	$25, 128($sp)
	lhu	$10, 14($7)
	beq	$25, $10, $192
	lw	$6, 100($sp)
	b	$194
$192:
	.loc	2 2142
 #2141		{
 #2142		    if (!pWin->parent)
	lw	$11, 24($7)
	bne	$11, $0, $193
	.loc	2 2143
 #2143			miDestroyBSPixmap (pWin);
	move	$4, $7
	jal	$$834
	lw	$7, 112($sp)
$193:
	lw	$6, 100($sp)
	.loc	2 2144
 #2144		    if (pBackingStore->status != StatusContents)
	lb	$12, 17($6)
	beq	$12, 5, $194
	.loc	2 2145
 #2145			 miTileVirtualBS (pWin);
	move	$4, $7
	jal	$$835
	lw	$6, 100($sp)
	lw	$7, 112($sp)
	.loc	2 2146
 #2146		}
$194:
	addu	$4, $sp, 44
	li	$5, 1
	.loc	2 2148
 #2147	
 #2148		ts_x_origin = ts_y_origin = 0;
	move	$8, $0
	move	$9, $0
	.loc	2 2150
 #2149	
 #2150		backgroundState = pWin->backgroundState;
	lw	$3, 124($7)
	and	$3, $3, 3
	sll	$3, $3, 24
	sra	$3, $3, 24
	.loc	2 2151
 #2151		background = pWin->background;
	.set	 noat
	lw	$1, 108($7)
	sw	$1, 0($4)
	.set	 at
	.loc	2 2152
 #2152		if (backgroundState == ParentRelative) {
	bne	$3, $5, $197
	li	$3, 1
	.bgnb	1445
	.loc	2 2155
 #2153		    WindowPtr	pParent;
 #2154	
 #2155		    pParent = pWin;
	move	$2, $7
	.loc	2 2156
 #2156		    while (pParent->backgroundState == ParentRelative) {
	lw	$14, 124($7)
	and	$15, $14, 3
	bne	$15, $3, $196
$195:
	.loc	2 2157
 #2157			ts_x_origin -= pParent->origin.x;
	lh	$24, 96($2)
	subu	$9, $9, $24
	.loc	2 2158
 #2158			ts_y_origin -= pParent->origin.y;
	lh	$25, 98($2)
	subu	$8, $8, $25
	.loc	2 2159
 #2159			pParent = pParent->parent;
	lw	$2, 24($2)
	.loc	2 2160
 #2160		    }
	.loc	2 2160
	lw	$10, 124($2)
	and	$11, $10, 3
	beq	$11, $3, $195
$196:
	.loc	2 2161
 #2161		    backgroundState = pParent->backgroundState;
	lw	$3, 124($2)
	and	$3, $3, 3
	sll	$3, $3, 24
	sra	$3, $3, 24
	.loc	2 2162
 #2162		    background = pParent->background;
	.set	 noat
	lw	$1, 108($2)
	sw	$1, 0($4)
	.set	 at
	.loc	2 2163
 #2163		}
	.endb	1447
$197:
	.loc	2 2170
 #2170				      background)))
	beq	$3, 0, $212
	lb	$13, 17($6)
	beq	$13, 5, $200
	lb	$2, 18($6)
	bne	$2, $3, $200
	beq	$2, 0, $212
	beq	$2, $5, $212
	bne	$2, 2, $198
	lw	$14, 20($6)
	lw	$15, 44($sp)
	seq	$2, $14, $15
	b	$199
$198:
	lw	$24, 20($6)
	lw	$25, 44($sp)
	seq	$2, $24, $25
$199:
	bne	$2, 0, $212
$200:
	.loc	2 2172
 #2171		{
 #2172		    if (!pBackingStore->pBackingPixmap)
	lw	$10, 0($6)
	bne	$10, $0, $201
	.loc	2 2173
 #2173			miCreateBSPixmap(pWin);
	move	$4, $7
	sb	$3, 43($sp)
	sw	$8, 84($sp)
	sw	$9, 88($sp)
	jal	$$833
	lb	$3, 43($sp)
	lw	$8, 84($sp)
	lw	$9, 88($sp)
	lw	$6, 100($sp)
	lw	$7, 112($sp)
$201:
	.loc	2 2175
 #2174	
 #2175		    pGC = GetScratchGC(pWin->drawable.depth, pScreen);
	lbu	$4, 2($7)
	lw	$5, 96($sp)
	sb	$3, 43($sp)
	sw	$8, 84($sp)
	sw	$9, 88($sp)
	jal	GetScratchGC
	lb	$3, 43($sp)
	lw	$8, 84($sp)
	lw	$9, 88($sp)
	move	$4, $2
	.loc	2 2176
 #2176		    if (pGC && pBackingStore->pBackingPixmap)
	beq	$2, 0, $211
	lw	$11, 100($sp)
	lw	$12, 0($11)
	beq	$12, $0, $211
	.loc	2 2185
 #2185			if (backgroundState == BackgroundPixel)
	bne	$3, 2, $202
	.loc	2 2187
 #2186			{
 #2187			    gcvalues[0] = (XID) background.pixel;
	lw	$13, 44($sp)
	sw	$13, 68($sp)
	.loc	2 2188
 #2188			    gcvalues[1] = FillSolid;
	sw	$0, 72($sp)
	.loc	2 2189
 #2189			    gcmask = GCForeground|GCFillStyle;
	li	$2, 260
	.loc	2 2190
 #2190			}
	b	$203
$202:
	.loc	2 2193
 #2191			else
 #2192			{
 #2193			    gcvalues[0] = FillTiled;
	li	$14, 1
	sw	$14, 68($sp)
	.loc	2 2194
 #2194			    gcvalues[1] = (XID) background.pixmap;
	lw	$15, 44($sp)
	sw	$15, 72($sp)
	.loc	2 2195
 #2195			    gcmask = GCFillStyle|GCTile;
	li	$2, 1280
	.loc	2 2196
 #2196			}
$203:
	.loc	2 2197
 #2197			gcvalues[2] = ts_x_origin;
	sw	$9, 76($sp)
	.loc	2 2198
 #2198			gcvalues[3] = ts_y_origin;
	sw	$8, 80($sp)
	.loc	2 2199
 #2199			gcmask |= GCTileStipXOrigin|GCTileStipYOrigin;
	or	$2, $2, 12288
	.loc	2 2200
 #2200			DoChangeGC(pGC, gcmask, gcvalues, TRUE);
	move	$5, $2
	addu	$6, $sp, 68
	li	$7, 1
	sw	$4, 92($sp)
	jal	DoChangeGC
	.loc	2 2201
 #2201			ValidateGC((DrawablePtr)pBackingStore->pBackingPixmap, pGC);
	lw	$24, 100($sp)
	lw	$4, 0($24)
	lw	$5, 92($sp)
	jal	ValidateGC
	.loc	2 2207
 #2202	    
 #2203			/*
 #2204			 * Figure out the array of rectangles to fill and fill them with
 #2205			 * PolyFillRect in the proper mode, as set in the GC above.
 #2206			 */
 #2207			numRects = REGION_NUM_RECTS(pRgn);
	lw	$25, 108($sp)
	lw	$3, 8($25)
	beq	$3, $0, $204
	lw	$6, 4($3)
	b	$205
$204:
	li	$6, 1
$205:
	.loc	2 2208
 #2208			rects = (xRectangle *)ALLOCATE_LOCAL(numRects*sizeof(xRectangle));
	mul	$4, $6, 8
	sw	$6, 36($sp)
	jal	Xalloc
	lw	$6, 36($sp)
	move	$7, $2
	.loc	2 2210
 #2209		    
 #2210			if (rects)
	beq	$2, 0, $210
	lw	$8, 108($sp)
	.loc	2 2213
 #2211			{
 #2212			    for (i = 0, pBox = REGION_RECTS(pRgn);
 #2213				 i < numRects;
	move	$5, $0
	lw	$3, 8($8)
	beq	$3, $0, $206
	addu	$4, $3, 8
	b	$207
$206:
	move	$4, $8
$207:
	.loc	2 2213
	ble	$6, 0, $209
	mul	$10, $5, 8
	addu	$3, $2, $10
$208:
	.loc	2 2216
 #2214				 i++, pBox++)
 #2215			    {
 #2216				rects[i].x = pBox->x1;
	lh	$11, 0($4)
	sh	$11, 0($3)
	.loc	2 2217
 #2217				rects[i].y = pBox->y1;
	lh	$12, 2($4)
	sh	$12, 2($3)
	.loc	2 2218
 #2218				rects[i].width = pBox->x2 - pBox->x1;
	lh	$13, 4($4)
	lh	$14, 0($4)
	subu	$15, $13, $14
	sh	$15, 4($3)
	.loc	2 2219
 #2219				rects[i].height = pBox->y2 - pBox->y1;
	lh	$24, 6($4)
	lh	$25, 2($4)
	subu	$10, $24, $25
	sh	$10, 6($3)
	.loc	2 2220
 #2220			    }
	.loc	2 2220
	addu	$5, $5, 1
	addu	$3, $3, 8
	addu	$4, $4, 8
	.loc	2 2220
	blt	$5, $6, $208
$209:
	lw	$5, 92($sp)
	.loc	2 2222
 #2221			    (* pGC->ops->PolyFillRect) (pBackingStore->pBackingPixmap,
 #2222					       pGC, numRects, rects);
	lw	$11, 100($sp)
	lw	$4, 0($11)
	sw	$7, 60($sp)
	lw	$12, 72($5)
	lw	$13, 44($12)
	jal	$13
	lw	$7, 60($sp)
	.loc	2 2223
 #2223			    DEALLOCATE_LOCAL(rects);
	move	$4, $7
	jal	Xfree
	.loc	2 2224
 #2224			}	
$210:
	.loc	2 2225
 #2225			FreeScratchGC(pGC);
	lw	$4, 92($sp)
	jal	FreeScratchGC
	.loc	2 2226
 #2226		    }
$211:
	.loc	2 2227
 #2227		}	
	lw	$7, 112($sp)
$212:
	.loc	2 2229
 #2228	
 #2229		if (!generateExposures)
	lw	$14, 132($sp)
	bne	$14, 0, $213
	.loc	2 2231
 #2230	 	{
 #2231		    (*pScreen->RegionDestroy) (pRgn);
	lw	$4, 108($sp)
	lw	$15, 96($sp)
	lw	$24, 316($15)
	jal	$24
	.loc	2 2232
 #2232		    pRgn = NULL;
	sw	$0, 108($sp)
	.loc	2 2233
 #2233		}
	b	$215
$213:
	.loc	2 2240
 #2240		    (*pScreen->TranslateRegion) (pRgn, pWin->drawable.x, pWin->drawable.y);
	lw	$4, 108($sp)
	lh	$5, 8($7)
	lh	$6, 10($7)
	lw	$25, 96($sp)
	lw	$10, 344($25)
	jal	$10
	.loc	2 2241
 #2241		}
	.loc	2 2242
 #2242	    }
	b	$215
$214:
	.loc	2 2245
 #2243	    else
 #2244	    {
 #2245		(* pScreen->RegionDestroy) (pRgn);
	lw	$4, 108($sp)
	lw	$11, 96($sp)
	lw	$12, 316($11)
	jal	$12
	.loc	2 2246
 #2246		pRgn = NULL;
	sw	$0, 108($sp)
	.loc	2 2247
 #2247	    }
$215:
	.loc	2 2248
 #2248	    return pRgn;
	lw	$2, 108($sp)
	.endb	1449
$216:
	lw	$31, 20($sp)
	addu	$sp, 112
	j	$31
	.end	miBSClearBackingStore
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2266
 #2266	{
	.ent	$$907 2
$$907:
	.option	O2
	subu	$sp, 72
	sw	$31, 20($sp)
	.mask	0x80000000, -52
	.frame	$sp, 72, $31
	sw	$4, 72($sp)
	move	$10, $5
	sw	$6, 80($sp)
	move	$11, $7
	.bgnb	1460
	.loc	2 2275
 #2275	    if (state == None)
	lw	$14, 92($sp)
	beq	$14, 0, $237
	.loc	2 2276
 #2276		return;
	.loc	2 2277
 #2277	    numRects = REGION_NUM_RECTS(pRgn);
	lw	$15, 80($sp)
	lw	$3, 8($15)
	beq	$3, $0, $217
	lw	$24, 4($3)
	sw	$24, 28($sp)
	b	$218
$217:
	li	$25, 1
	sw	$25, 28($sp)
$218:
	.loc	2 2278
 #2278	    pRect = (xRectangle *)ALLOCATE_LOCAL(numRects * sizeof(xRectangle));
	lw	$4, 28($sp)
	mul	$4, $4, 8
	sw	$10, 76($sp)
	sw	$11, 84($sp)
	jal	Xalloc
	lw	$10, 76($sp)
	lw	$11, 84($sp)
	move	$8, $2
	.loc	2 2279
 #2279	    if (!pRect)
	beq	$2, 0, $237
	.loc	2 2280
 #2280		return;
	lw	$13, 72($sp)
	.loc	2 2281
 #2281	    pWin = 0;
	move	$12, $0
	.loc	2 2282
 #2282	    if (pDrawable->type == DRAWABLE_WINDOW)
	lbu	$14, 0($13)
	bne	$14, 0, $219
	.loc	2 2284
 #2283	    {
 #2284		pWin = (WindowPtr) pDrawable;
	move	$12, $13
	.loc	2 2285
 #2285		if (!pWin->backStorage)
	lw	$15, 116($13)
	bne	$15, $0, $219
	.loc	2 2286
 #2286		    pWin = 0;
	move	$12, $0
	.loc	2 2287
 #2287	    }
$219:
	.loc	2 2288
 #2288	    i = 0;
	.loc	2 2289
 #2289	    gcmask = 0;
	.loc	2 2290
 #2290	    gcval[i++] = planeMask;
	lw	$24, 100($sp)
	sw	$24, 44($sp)
	li	$3, 1
	.loc	2 2291
 #2291	    gcmask |= GCPlaneMask;
	li	$5, 2
	.loc	2 2292
 #2292	    if (state == BackgroundPixel)
	lw	$25, 92($sp)
	bne	$25, 2, $222
	lw	$2, 96($sp)
	.loc	2 2294
 #2293	    {
 #2294		if (pGC->fgPixel != pixunion.pixel)
	lw	$14, 24($10)
	beq	$14, $2, $220
	.loc	2 2296
 #2295		{
 #2296		    gcval[i++] = (XID) pixunion.pixel;
	sw	$2, 48($sp)
	li	$3, 2
	.loc	2 2297
 #2297		    gcmask |= GCForeground;
	li	$5, 6
	.loc	2 2298
 #2298		}
$220:
	.loc	2 2299
 #2299		if (pGC->fillStyle != FillSolid)
	lw	$15, 16($10)
	sll	$24, $15, 24
	srl	$25, $24, 30
	beq	$25, 0, $221
	.loc	2 2301
 #2300		{
 #2301		    gcval[i++] = (XID) FillSolid;
	mul	$14, $3, 4
	addu	$15, $sp, $14
	sw	$0, 44($15)
	.loc	2 2302
 #2302		    gcmask |= GCFillStyle;
	or	$5, $5, 256
	.loc	2 2303
 #2303		}
$221:
	.loc	2 2304
 #2304	    }
	lw	$9, 88($sp)
	b	$228
$222:
	.loc	2 2307
 #2305	    else
 #2306	    {
 #2307		if (pGC->fillStyle != FillTiled)
	lw	$2, 16($10)
	sll	$24, $2, 24
	srl	$25, $24, 30
	beq	$25, 1, $223
	.loc	2 2309
 #2308		{
 #2309		    gcval[i++] = (XID) FillTiled;
	li	$14, 1
	sw	$14, 48($sp)
	li	$3, 2
	.loc	2 2310
 #2310		    gcmask |= GCFillStyle;
	li	$5, 258
	.loc	2 2311
 #2311		}
	lw	$2, 16($10)
$223:
	.loc	2 2312
 #2312		if (pGC->tileIsPixel || pGC->tile.pixmap != pixunion.pixmap)
	sll	$15, $2, 16
	srl	$24, $15, 31
	beq	$24, $0, $224
	lw	$2, 96($sp)
	b	$225
$224:
	lw	$2, 96($sp)
	lw	$25, 32($10)
	beq	$25, $2, $226
$225:
	.loc	2 2314
 #2313		{
 #2314		    gcval[i++] = (XID) pixunion.pixmap;
	mul	$14, $3, 4
	addu	$15, $sp, $14
	sw	$2, 44($15)
	addu	$3, $3, 1
	.loc	2 2315
 #2315		    gcmask |= GCTile;
	or	$5, $5, 1024
	.loc	2 2316
 #2316		}
$226:
	.loc	2 2317
 #2317		if (pGC->patOrg.x != -x)
	negu	$2, $11
	lh	$24, 40($10)
	beq	$24, $2, $227
	.loc	2 2319
 #2318		{
 #2319		    gcval[i++] = (XID) -x;
	mul	$25, $3, 4
	addu	$14, $sp, $25
	sw	$2, 44($14)
	addu	$3, $3, 1
	.loc	2 2320
 #2320		    gcmask |= GCTileStipXOrigin;
	or	$5, $5, 4096
	.loc	2 2321
 #2321		}
$227:
	lw	$9, 88($sp)
	.loc	2 2322
 #2322		if (pGC->patOrg.y != -y)
	negu	$2, $9
	lh	$15, 42($10)
	beq	$15, $2, $228
	.loc	2 2324
 #2323		{
 #2324		    gcval[i++] = (XID) -y;
	mul	$24, $3, 4
	addu	$25, $sp, $24
	sw	$2, 44($25)
	.loc	2 2325
 #2325		    gcmask |= GCTileStipYOrigin;
	or	$5, $5, 8192
	.loc	2 2326
 #2326		}
	.loc	2 2327
 #2327	    }
$228:
	.loc	2 2328
 #2328	    if (gcmask)
	beq	$5, 0, $229
	.loc	2 2329
 #2329		DoChangeGC (pGC, gcmask, gcval, 1);
	move	$4, $10
	addu	$6, $sp, 44
	li	$7, 1
	sw	$8, 40($sp)
	sw	$10, 76($sp)
	sw	$11, 84($sp)
	sw	$12, 32($sp)
	jal	DoChangeGC
	lw	$8, 40($sp)
	lw	$9, 88($sp)
	lw	$10, 76($sp)
	lw	$11, 84($sp)
	lw	$12, 32($sp)
	lw	$13, 72($sp)
$229:
	.loc	2 2331
 #2330	
 #2331	    if (pWin)
	beq	$12, 0, $230
	.loc	2 2332
 #2332		(*pWin->drawable.pScreen->DrawGuarantee) (pWin, pGC, GuaranteeVisBack);
	move	$4, $12
	move	$5, $10
	li	$6, 1
	sw	$8, 40($sp)
	sw	$10, 76($sp)
	sw	$11, 84($sp)
	sw	$12, 32($sp)
	lw	$14, 16($12)
	lw	$15, 232($14)
	jal	$15
	lw	$8, 40($sp)
	lw	$9, 88($sp)
	lw	$10, 76($sp)
	lw	$11, 84($sp)
	lw	$12, 32($sp)
	lw	$13, 72($sp)
$230:
	.loc	2 2334
 #2333	
 #2334	    if (pDrawable->serialNumber != pGC->serialNumber)
	lw	$24, 20($13)
	lw	$25, 64($10)
	beq	$24, $25, $231
	.loc	2 2335
 #2335		ValidateGC (pDrawable, pGC);
	move	$4, $13
	move	$5, $10
	sw	$8, 40($sp)
	sw	$10, 76($sp)
	sw	$11, 84($sp)
	sw	$12, 32($sp)
	jal	ValidateGC
	lw	$8, 40($sp)
	lw	$9, 88($sp)
	lw	$10, 76($sp)
	lw	$11, 84($sp)
	lw	$12, 32($sp)
	lw	$13, 72($sp)
$231:
	lw	$4, 80($sp)
	.loc	2 2337
 #2336	
 #2337	    pBox = REGION_RECTS(pRgn);
	lw	$3, 8($4)
	beq	$3, $0, $232
	addu	$2, $3, 8
	b	$233
$232:
	move	$2, $4
$233:
	lw	$6, 28($sp)
	.loc	2 2338
 #2338	    for (i = numRects; --i >= 0; pBox++, pRect++)
	.loc	2 2338
	addu	$3, $6, -1
	blt	$3, 0, $235
$234:
	.loc	2 2340
 #2339	    {
 #2340	    	pRect->x = pBox->x1 - x;
	lh	$14, 0($2)
	subu	$15, $14, $11
	sh	$15, 0($8)
	.loc	2 2341
 #2341		pRect->y = pBox->y1 - y;
	lh	$24, 2($2)
	subu	$25, $24, $9
	sh	$25, 2($8)
	.loc	2 2342
 #2342		pRect->width = pBox->x2 - pBox->x1;
	lh	$14, 4($2)
	lh	$15, 0($2)
	subu	$24, $14, $15
	sh	$24, 4($8)
	.loc	2 2343
 #2343		pRect->height = pBox->y2 - pBox->y1;
	lh	$25, 6($2)
	lh	$14, 2($2)
	subu	$15, $25, $14
	sh	$15, 6($8)
	.loc	2 2344
 #2344	    }
	.loc	2 2344
	addu	$2, $2, 8
	addu	$8, $8, 8
	.loc	2 2344
	addu	$3, $3, -1
	bge	$3, 0, $234
$235:
	.loc	2 2345
 #2345	    pRect -= numRects;
	negu	$24, $6
	mul	$25, $24, 8
	addu	$8, $8, $25
	.loc	2 2346
 #2346	    (*pGC->ops->PolyFillRect) (pDrawable, pGC, numRects, pRect);
	move	$4, $13
	move	$5, $10
	move	$7, $8
	sw	$8, 40($sp)
	sw	$10, 76($sp)
	sw	$12, 32($sp)
	lw	$14, 72($10)
	lw	$15, 44($14)
	jal	$15
	lw	$8, 40($sp)
	lw	$10, 76($sp)
	lw	$12, 32($sp)
	.loc	2 2347
 #2347	    if (pWin)
	beq	$12, 0, $236
	.loc	2 2348
 #2348		(*pWin->drawable.pScreen->DrawGuarantee) (pWin, pGC, GuaranteeNothing);
	move	$4, $12
	move	$5, $10
	move	$6, $0
	sw	$8, 40($sp)
	lw	$24, 16($12)
	lw	$25, 232($24)
	jal	$25
	lw	$8, 40($sp)
$236:
	.loc	2 2349
 #2349	    DEALLOCATE_LOCAL (pRect);
	move	$4, $8
	jal	Xfree
	.loc	2 2350
 #2350	}
	.endb	1468
$237:
	lw	$31, 20($sp)
	addu	$sp, 72
	j	$31
	.end	miBSFillVirtualBits
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2363
 #2363	{
	.ent	$$836 2
$$836:
	.option	O2
	subu	$sp, 80
	sw	$31, 20($sp)
	sw	$16, 16($sp)
	.mask	0x80010000, -60
	.frame	$sp, 80, $31
	move	$16, $4
	.bgnb	1472
	.loc	2 2368
 #2364	    register miBSWindowPtr  pBackingStore;
 #2365	    register ScreenPtr 	    pScreen;
 #2366		
 #2367	    if (!pWin->backStorage &&
 #2368		(pWin->drawable.pScreen->backingStoreSupport != NotUseful))
	lw	$14, 116($16)
	bne	$14, $0, $256
	lw	$2, 16($16)
	lb	$15, 36($2)
	beq	$15, 0, $256
	.loc	2 2370
 #2369	    {
 #2370		pScreen = pWin->drawable.pScreen;
	sw	$2, 72($sp)
	.loc	2 2372
 #2371	
 #2372		pBackingStore = (miBSWindowPtr)xalloc(sizeof(miBSWindowRec));
	li	$4, 24
	jal	Xalloc
	move	$3, $2
	.loc	2 2373
 #2373		if (!pBackingStore)
	beq	$2, 0, $256
	.loc	2 2374
 #2374		    return;
	.loc	2 2376
 #2375	
 #2376		pBackingStore->pBackingPixmap = NullPixmap;
	sw	$0, 0($3)
	.loc	2 2377
 #2377		(* pScreen->RegionInit)(&pBackingStore->SavedRegion, NullBox, 1);
	addu	$24, $3, 4
	sw	$24, 32($sp)
	move	$4, $24
	move	$5, $0
	li	$6, 1
	sw	$3, 76($sp)
	lw	$25, 72($sp)
	lw	$9, 308($25)
	jal	$9
	lw	$3, 76($sp)
	.loc	2 2378
 #2378		pBackingStore->viewable = (char)pWin->viewable;
	lw	$10, 124($16)
	sll	$11, $10, 10
	srl	$12, $11, 31
	sb	$12, 16($3)
	.loc	2 2379
 #2379		pBackingStore->status = StatusNoPixmap;
	li	$13, 1
	sb	$13, 17($3)
	.loc	2 2380
 #2380		pBackingStore->backgroundState = None;
	sb	$0, 18($3)
	.loc	2 2382
 #2381		
 #2382		pWin->backStorage = (pointer) pBackingStore;
	sw	$3, 116($16)
	.loc	2 2393
 #2393		    (pWin->backingStore == Always))
	lw	$2, 124($16)
	sll	$4, $2, 26
	srl	$4, $4, 30
	bne	$4, 1, $238
	sll	$14, $2, 10
	srl	$15, $14, 31
	bne	$15, $0, $239
$238:
	bne	$4, 2, $256
$239:
	.bgnb	1475
	.loc	2 2402
 #2402		    pSavedRegion = &pBackingStore->SavedRegion;
	lw	$2, 32($sp)
	.loc	2 2404
 #2403	
 #2404		    box.x1 = pWin->drawable.x;
	lh	$24, 8($16)
	sh	$24, 60($sp)
	.loc	2 2405
 #2405		    box.x2 = box.x1 + (int) pWin->drawable.width;
	lh	$25, 60($sp)
	lhu	$9, 12($16)
	addu	$10, $25, $9
	sh	$10, 64($sp)
	.loc	2 2406
 #2406		    box.y1 = pWin->drawable.y;
	lh	$11, 10($16)
	sh	$11, 62($sp)
	.loc	2 2407
 #2407		    box.y2 = pWin->drawable.y + (int) pWin->drawable.height;
	lh	$12, 10($16)
	lhu	$13, 14($16)
	addu	$14, $12, $13
	sh	$14, 66($sp)
	.loc	2 2409
 #2408	
 #2409		    (* pScreen->Inverse)(pSavedRegion, &pWin->clipList,  &box);
	move	$4, $2
	addu	$5, $16, 44
	addu	$6, $sp, 60
	sw	$2, 56($sp)
	lw	$15, 72($sp)
	lw	$24, 336($15)
	jal	$24
	.loc	2 2412
 #2410		    (* pScreen->TranslateRegion) (pSavedRegion,
 #2411						  -pWin->drawable.x,
 #2412						  -pWin->drawable.y);
	lw	$4, 56($sp)
	lh	$5, 8($16)
	negu	$5, $5
	lh	$6, 10($16)
	negu	$6, $6
	lw	$25, 72($sp)
	lw	$9, 344($25)
	jal	$9
	.loc	2 2414
 #2413	#ifdef SHAPE
 #2414		    if (wBoundingShape (pWin))
	lw	$2, 120($16)
	beq	$2, $0, $240
	lw	$6, 40($2)
	b	$241
$240:
	move	$6, $0
$241:
	beq	$6, 0, $244
	.loc	2 2415
 #2415			(*pScreen->Intersect) (pSavedRegion, pSavedRegion, wBoundingShape (pWin));
	beq	$2, $0, $242
	lw	$6, 40($2)
	b	$243
$242:
	move	$6, $0
$243:
	lw	$4, 56($sp)
	move	$5, $4
	lw	$10, 72($sp)
	lw	$11, 324($10)
	jal	$11
	lw	$2, 120($16)
$244:
	.loc	2 2416
 #2416		    if (wClipShape (pWin))
	beq	$2, $0, $245
	lw	$6, 44($2)
	b	$246
$245:
	move	$6, $0
$246:
	beq	$6, 0, $249
	.loc	2 2417
 #2417			(*pScreen->Intersect) (pSavedRegion, pSavedRegion, wClipShape (pWin));
	beq	$2, $0, $247
	lw	$6, 44($2)
	b	$248
$247:
	move	$6, $0
$248:
	lw	$4, 56($sp)
	move	$5, $4
	lw	$12, 72($sp)
	lw	$13, 324($12)
	jal	$13
$249:
	.loc	2 2419
 #2418	#endif
 #2419		    miTileVirtualBS (pWin);
	move	$4, $16
	jal	$$835
	.loc	2 2426
 #2426		    pBox = REGION_RECTS(pSavedRegion);
	lw	$14, 56($sp)
	lw	$2, 8($14)
	beq	$2, $0, $250
	addu	$5, $2, 8
	b	$251
$250:
	lw	$5, 56($sp)
$251:
	.loc	2 2427
 #2427		    numRects = REGION_NUM_RECTS(pSavedRegion);
	beq	$2, $0, $252
	lw	$6, 4($2)
	b	$253
$252:
	li	$6, 1
$253:
	.loc	2 2429
 #2428		    if(!(pEvent = (xEvent *) ALLOCATE_LOCAL(numRects *
 #2429							    sizeof(xEvent))))
	mul	$4, $6, 32
	sw	$5, 68($sp)
	sw	$6, 40($sp)
	jal	Xalloc
	lw	$5, 68($sp)
	lw	$6, 40($sp)
	move	$8, $2
	beq	$2, 0, $256
	.loc	2 2430
 #2430			return;
	.loc	2 2432
 #2431	
 #2432		    for (i=numRects, pe = pEvent; --i >= 0; pe++, pBox++)
	move	$3, $2
	.loc	2 2432
	addu	$4, $6, -1
	blt	$4, 0, $255
	li	$2, 12
$254:
	.loc	2 2434
 #2433		    {
 #2434			pe->u.u.type = Expose;
	sb	$2, 0($3)
	.loc	2 2435
 #2435			pe->u.expose.window = pWin->drawable.id;
	lw	$15, 4($16)
	sw	$15, 4($3)
	.loc	2 2436
 #2436			pe->u.expose.x = pBox->x1;
	lh	$24, 0($5)
	sh	$24, 8($3)
	.loc	2 2437
 #2437			pe->u.expose.y = pBox->y1;
	lh	$25, 2($5)
	sh	$25, 10($3)
	.loc	2 2438
 #2438			pe->u.expose.width = pBox->x2 - pBox->x1;
	lh	$9, 4($5)
	lh	$10, 0($5)
	subu	$11, $9, $10
	sh	$11, 12($3)
	.loc	2 2439
 #2439			pe->u.expose.height = pBox->y2 - pBox->y1;
	lh	$12, 6($5)
	lh	$13, 2($5)
	subu	$14, $12, $13
	sh	$14, 14($3)
	.loc	2 2440
 #2440			pe->u.expose.count = i;
	sh	$4, 16($3)
	.loc	2 2441
 #2441		    }
	.loc	2 2441
	addu	$3, $3, 32
	addu	$5, $5, 8
	.loc	2 2441
	addu	$4, $4, -1
	bge	$4, 0, $254
$255:
	.loc	2 2442
 #2442		    DeliverEvents(pWin, pEvent, numRects, NullWindow);
	move	$4, $16
	move	$5, $8
	move	$7, $0
	sw	$8, 52($sp)
	jal	DeliverEvents
	lw	$8, 52($sp)
	.loc	2 2443
 #2443		    DEALLOCATE_LOCAL(pEvent);
	move	$4, $8
	jal	Xfree
	.loc	2 2444
 #2444		}
	.endb	1484
	.loc	2 2445
 #2445	    }
	.loc	2 2446
 #2446	}
	.endb	1485
$256:
	lw	$16, 16($sp)
	lw	$31, 20($sp)
	addu	$sp, 80
	j	$31
	.end	miBSAllocate
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2465
 #2465	{
	.ent	$$837 2
$$837:
	.option	O2
	subu	$sp, 40
	sw	$31, 20($sp)
	.mask	0x80000000, -20
	.frame	$sp, 40, $31
	.bgnb	1489
	.loc	2 2467
 #2466	    miBSWindowPtr 	pBackingStore;
 #2467	    register ScreenPtr	pScreen = pWin->drawable.pScreen;
	lw	$14, 16($4)
	sw	$14, 32($sp)
	.loc	2 2469
 #2468	
 #2469	    if (pWin->backStorage)
	lw	$2, 116($4)
	beq	$2, $0, $259
	sw	$16, 24($sp)
	.loc	2 2471
 #2470	    {
 #2471		pBackingStore = (miBSWindowPtr)pWin->backStorage;
	move	$16, $2
	.loc	2 2472
 #2472		if (pBackingStore)
	beq	$16, 0, $258
	.loc	2 2474
 #2473		{
 #2474		    miDestroyBSPixmap (pWin);
	sw	$4, 40($sp)
	jal	$$834
	.loc	2 2476
 #2475	    
 #2476		    (* pScreen->RegionUninit)(&pBackingStore->SavedRegion);
	addu	$4, $16, 4
	lw	$15, 32($sp)
	lw	$24, 320($15)
	jal	$24
	.loc	2 2478
 #2477	
 #2478		    if (pBackingStore->backgroundState == BackgroundPixmap)
	lb	$25, 18($16)
	bne	$25, 3, $257
	.loc	2 2479
 #2479			(*pScreen->DestroyPixmap) (pBackingStore->background.pixmap);
	lw	$4, 20($16)
	lw	$8, 32($sp)
	lw	$9, 208($8)
	jal	$9
$257:
	.loc	2 2481
 #2480	
 #2481		    xfree(pBackingStore);
	move	$4, $16
	jal	Xfree
	.loc	2 2482
 #2482		    pWin->backStorage = NULL;
	lw	$10, 40($sp)
	sw	$0, 116($10)
	.loc	2 2483
 #2483		}
$258:
	.loc	2 2484
 #2484	    }
	lw	$16, 24($sp)
	.loc	2 2485
 #2485	}
	.endb	1492
$259:
	lw	$31, 20($sp)
	addu	$sp, 40
	j	$31
	.end	miBSFree
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2509
 #2509	{
	.ent	$$1495 2
$$1495:
	.option	O2
	subu	$sp, 96
	sw	$31, 44($sp)
	sw	$16, 40($sp)
	.mask	0x80010000, -52
	.frame	$sp, 96, $31
	move	$3, $4
	sw	$5, 100($sp)
	sw	$6, 104($sp)
	.bgnb	1499
	.loc	2 2518
 #2518	    pBackingStore = (miBSWindowPtr)(pWin->backStorage);
	lw	$14, 116($3)
	sw	$14, 92($sp)
	.loc	2 2519
 #2519	    pScreen = pWin->drawable.pScreen;
	lw	$16, 16($3)
	.loc	2 2520
 #2520	    pBackingPixmap = pBackingStore->pBackingPixmap;
	lw	$15, 92($sp)
	lw	$2, 0($15)
	.loc	2 2522
 #2521	
 #2522	    if (pBackingPixmap)
	beq	$2, 0, $262
	.bgnb	1507
	.loc	2 2530
 #2530						 pWin->drawable.depth);
	move	$4, $16
	lhu	$5, 12($3)
	lhu	$6, 14($3)
	lbu	$7, 2($3)
	sw	$2, 88($sp)
	sw	$3, 96($sp)
	lw	$24, 204($16)
	jal	$24
	sw	$2, 60($sp)
	.loc	2 2532
 #2531	
 #2532		if (pNewPixmap)
	beq	$2, 0, $260
	.loc	2 2534
 #2533		{
 #2534		    if ((* pScreen->RegionNotEmpty) (&pBackingStore->SavedRegion))
	lw	$25, 92($sp)
	addu	$8, $25, 4
	sw	$8, 52($sp)
	move	$4, $8
	lw	$9, 356($16)
	jal	$9
	beq	$2, 0, $261
	.loc	2 2536
 #2535		    {
 #2536			extents = (*pScreen->RegionExtents)(&pBackingStore->SavedRegion);
	lw	$4, 52($sp)
	lw	$10, 364($16)
	jal	$10
	move	$3, $2
	.loc	2 2537
 #2537			pGC = GetScratchGC(pNewPixmap->drawable.depth, pScreen);
	lw	$11, 60($sp)
	lbu	$4, 2($11)
	move	$5, $16
	sw	$3, 76($sp)
	jal	GetScratchGC
	lw	$3, 76($sp)
	move	$6, $2
	.loc	2 2538
 #2538			if (pGC)
	beq	$2, 0, $261
	.loc	2 2540
 #2539			{
 #2540			    ValidateGC((DrawablePtr)pNewPixmap, pGC);
	lw	$4, 60($sp)
	move	$5, $6
	sw	$3, 76($sp)
	sw	$6, 80($sp)
	jal	ValidateGC
	lw	$3, 76($sp)
	lw	$6, 80($sp)
	.loc	2 2545
 #2541			    (*pGC->ops->CopyArea)(pBackingPixmap, pNewPixmap, pGC,
 #2542					     extents->x1, extents->y1,
 #2543					     extents->x2 - extents->x1,
 #2544					     extents->y2 - extents->y1,
 #2545					     extents->x1 + dx, extents->y1 + dy);
	lw	$4, 88($sp)
	lw	$5, 60($sp)
	lh	$7, 0($3)
	lh	$2, 2($3)
	sw	$2, 16($sp)
	lh	$12, 4($3)
	subu	$13, $12, $7
	sw	$13, 20($sp)
	lh	$14, 6($3)
	subu	$15, $14, $2
	sw	$15, 24($sp)
	lw	$24, 100($sp)
	addu	$25, $7, $24
	sw	$25, 28($sp)
	lw	$8, 104($sp)
	addu	$9, $2, $8
	sw	$9, 32($sp)
	lw	$10, 72($6)
	lw	$11, 12($10)
	jal	$11
	lw	$6, 80($sp)
	.loc	2 2546
 #2546			    FreeScratchGC(pGC);
	move	$4, $6
	jal	FreeScratchGC
	.loc	2 2547
 #2547			}
	.loc	2 2548
 #2548		    }
	.loc	2 2549
 #2549		}
	b	$261
$260:
	.loc	2 2552
 #2550		else
 #2551		{
 #2552		    pBackingStore->status = StatusNoPixmap;
	li	$12, 1
	lw	$13, 92($sp)
	sb	$12, 17($13)
	.loc	2 2553
 #2553		}
$261:
	.loc	2 2555
 #2554	
 #2555		(* pScreen->DestroyPixmap)(pBackingPixmap);
	lw	$4, 88($sp)
	lw	$14, 208($16)
	jal	$14
	.loc	2 2556
 #2556		pBackingStore->pBackingPixmap = pNewPixmap;
	lw	$15, 60($sp)
	lw	$24, 92($sp)
	sw	$15, 0($24)
	.loc	2 2557
 #2557	    }
	.endb	1509
	lw	$3, 96($sp)
$262:
	.loc	2 2563
 #2558	
 #2559	    /*
 #2560	     * Now we need to translate SavedRegion, as appropriate, and clip it
 #2561	     * to be within the window's new bounds.
 #2562	     */
 #2563	    if (dx || dy)
	lw	$25, 92($sp)
	addu	$8, $25, 4
	sw	$8, 52($sp)
	lw	$9, 100($sp)
	bne	$9, 0, $263
	lw	$10, 104($sp)
	bne	$10, 0, $263
	sw	$3, 96($sp)
	b	$264
$263:
	.loc	2 2566
 #2564	    {
 #2565		(* pWin->drawable.pScreen->TranslateRegion)
 #2566					(&pBackingStore->SavedRegion, dx, dy);
	lw	$4, 52($sp)
	lw	$5, 100($sp)
	lw	$6, 104($sp)
	sw	$3, 96($sp)
	lw	$11, 16($3)
	lw	$12, 344($11)
	jal	$12
	.loc	2 2567
 #2567	    }
$264:
	lw	$3, 96($sp)
	.loc	2 2568
 #2568	    pixbounds.x1 = 0;
	sh	$0, 68($sp)
	.loc	2 2569
 #2569	    pixbounds.x2 = pWin->drawable.width;
	lhu	$13, 12($3)
	sh	$13, 72($sp)
	.loc	2 2570
 #2570	    pixbounds.y1 = 0;
	sh	$0, 70($sp)
	.loc	2 2571
 #2571	    pixbounds.y2 = pWin->drawable.height;
	lhu	$14, 14($3)
	sh	$14, 74($sp)
	.loc	2 2572
 #2572	    prgnTmp = (* pScreen->RegionCreate)(&pixbounds, 1);
	addu	$4, $sp, 68
	li	$5, 1
	lw	$15, 304($16)
	jal	$15
	move	$7, $2
	.loc	2 2574
 #2573	#ifdef SHAPE
 #2574	    if (wBoundingShape (pWin))
	lw	$24, 96($sp)
	lw	$3, 120($24)
	beq	$3, $0, $265
	lw	$6, 40($3)
	b	$266
$265:
	move	$6, $0
$266:
	beq	$6, 0, $269
	.loc	2 2575
 #2575		(*pScreen->Intersect) (prgnTmp, prgnTmp, wBoundingShape (pWin));
	beq	$3, $0, $267
	lw	$6, 40($3)
	b	$268
$267:
	move	$6, $0
$268:
	move	$4, $7
	move	$5, $7
	sw	$7, 64($sp)
	lw	$25, 324($16)
	jal	$25
	lw	$7, 64($sp)
	lw	$8, 96($sp)
	lw	$3, 120($8)
$269:
	.loc	2 2576
 #2576	    if (wClipShape (pWin))
	beq	$3, $0, $270
	lw	$6, 44($3)
	b	$271
$270:
	move	$6, $0
$271:
	beq	$6, 0, $274
	.loc	2 2577
 #2577		(*pScreen->Intersect) (prgnTmp, prgnTmp, wClipShape (pWin));
	beq	$3, $0, $272
	lw	$6, 44($3)
	b	$273
$272:
	move	$6, $0
$273:
	move	$4, $7
	move	$5, $7
	sw	$7, 64($sp)
	lw	$9, 324($16)
	jal	$9
	lw	$7, 64($sp)
$274:
	.loc	2 2581
 #2578	#endif
 #2579	    (* pScreen->Intersect)(&pBackingStore->SavedRegion,
 #2580				   &pBackingStore->SavedRegion,
 #2581				   prgnTmp);
	lw	$10, 52($sp)
	move	$4, $10
	move	$5, $10
	move	$6, $7
	sw	$7, 64($sp)
	lw	$11, 324($16)
	jal	$11
	lw	$7, 64($sp)
	.loc	2 2582
 #2582	    (* pScreen->RegionDestroy)(prgnTmp);
	move	$4, $7
	lw	$12, 316($16)
	jal	$12
	.loc	2 2583
 #2583	}
	.endb	1510
	lw	$16, 40($sp)
	lw	$31, 44($sp)
	addu	$sp, 96
	j	$31
	.end	miResizeBackingStore
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2607
 #2607	{
	.ent	$$847 2
$$847:
	.option	O2
	subu	$sp, 48
	sw	$31, 28($sp)
	sw	$18, 24($sp)
	sd	$16, 16($sp)
	.mask	0x80070000, -20
	.frame	$sp, 48, $31
	move	$16, $4
	sw	$5, 52($sp)
	sw	$6, 56($sp)
	sw	$7, 60($sp)
	.bgnb	1516
	.loc	2 2612
 #2608	    miBSWindowPtr 	pBackingStore;
 #2609	    ScreenPtr	  	pScreen;
 #2610	    
 #2611	
 #2612	    pBackingStore = (miBSWindowPtr)pWin->backStorage;
	lw	$17, 116($16)
	.loc	2 2613
 #2613	    pScreen = pWin->drawable.pScreen;
	lw	$18, 16($16)
	.loc	2 2619
 #2614	
 #2615	    /*
 #2616	     * If the window isn't realized, it's being unmapped, thus we don't
 #2617	     * want to save anything if backingStore isn't Always.
 #2618	     */
 #2619	    if (!pWin->realized)
	lw	$2, 124($16)
	sll	$14, $2, 11
	srl	$15, $14, 31
	bne	$15, $0, $275
	.loc	2 2621
 #2620	    {
 #2621		pBackingStore->viewable = (char)pWin->viewable;
	sll	$24, $2, 10
	srl	$25, $24, 31
	sb	$25, 16($17)
	.loc	2 2622
 #2622		if (pWin->backingStore != Always)
	lw	$8, 124($16)
	sll	$9, $8, 26
	srl	$10, $9, 30
	beq	$10, 2, $275
	.loc	2 2624
 #2623		{
 #2624		    (* pScreen->RegionEmpty) (&pBackingStore->SavedRegion);
	addu	$4, $17, 4
	lw	$11, 360($18)
	jal	$11
	.loc	2 2625
 #2625		    miDestroyBSPixmap (pWin);
	move	$4, $16
	jal	$$834
	.loc	2 2626
 #2626		    return;
	b	$279
$275:
	.loc	2 2632
 #2627		}
 #2628	    }
 #2629	
 #2630	    /* Don't even pretend to save anything for a virtual background None */
 #2631	    if ((pBackingStore->status == StatusVirtual) &&
 #2632		(pBackingStore->backgroundState == None))
	lb	$12, 17($17)
	bne	$12, 2, $276
	lb	$13, 18($17)
	beq	$13, 0, $279
	.loc	2 2633
 #2633		return;
$276:
	.loc	2 2635
 #2634	
 #2635	    if ((*pScreen->RegionNotEmpty)(pObscured))
	lw	$4, 52($sp)
	lw	$14, 356($18)
	jal	$14
	beq	$2, 0, $279
	.loc	2 2638
 #2636	    {
 #2637		(*pScreen->TranslateRegion) (pObscured,
 #2638					     -pWin->drawable.x, -pWin->drawable.y);
	lw	$4, 52($sp)
	lh	$5, 8($16)
	negu	$5, $5
	lh	$6, 10($16)
	negu	$6, $6
	lw	$15, 344($18)
	jal	$15
	.loc	2 2643
 #2639		/*
 #2640		 * only save the bits if we've actually
 #2641		 * started using backing store
 #2642		 */
 #2643		if (pBackingStore->status != StatusVirtual)
	lb	$24, 17($17)
	beq	$24, 2, $278
	.bgnb	1519
	.loc	2 2647
 #2644		{
 #2645		    miBSScreenPtr	pScreenPriv;
 #2646	
 #2647		    pScreenPriv = (miBSScreenPtr) pScreen->devPrivates[miBSScreenIndex].ptr;
	lw	$25, 404($18)
	lw	$8, $$839
	mul	$9, $8, 4
	addu	$10, $25, $9
	lw	$11, 0($10)
	sw	$11, 36($sp)
	.loc	2 2648
 #2648		    if (!pBackingStore->pBackingPixmap)
	lw	$4, 0($17)
	bne	$4, $0, $277
	.loc	2 2649
 #2649			miCreateBSPixmap (pWin);
	move	$4, $16
	jal	$$833
	lw	$4, 0($17)
$277:
	.loc	2 2651
 #2650	
 #2651		    if (pBackingStore->pBackingPixmap)
	beq	$4, $0, $278
	.loc	2 2655
 #2652			(* pScreenPriv->funcs->SaveAreas) (pBackingStore->pBackingPixmap,
 #2653							   pObscured,
 #2654							   pWin->drawable.x - dx,
 #2655							   pWin->drawable.y - dy);
	lw	$5, 52($sp)
	lh	$12, 8($16)
	lw	$13, 56($sp)
	subu	$6, $12, $13
	lh	$14, 10($16)
	lw	$15, 60($sp)
	subu	$7, $14, $15
	lw	$24, 36($sp)
	lw	$8, 24($24)
	lw	$25, 0($8)
	jal	$25
	.loc	2 2656
 #2656		}
	.endb	1521
$278:
	.loc	2 2659
 #2657		(* pScreen->Union)(&pBackingStore->SavedRegion,
 #2658				   &pBackingStore->SavedRegion,
 #2659				   pObscured);
	addu	$4, $17, 4
	move	$5, $4
	lw	$6, 52($sp)
	lw	$9, 328($18)
	jal	$9
	.loc	2 2661
 #2660		(*pScreen->TranslateRegion) (pObscured,
 #2661					     pWin->drawable.x, pWin->drawable.y);
	lw	$4, 52($sp)
	lh	$5, 8($16)
	lh	$6, 10($16)
	lw	$10, 344($18)
	jal	$10
	.loc	2 2662
 #2662	    }
	.loc	2 2663
 #2663	}
	.endb	1522
$279:
	ld	$16, 16($sp)
	lw	$18, 24($sp)
	lw	$31, 28($sp)
	addu	$sp, 48
	j	$31
	.end	miBSSaveDoomedAreas
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2693
 #2693	{
	.ent	$$848 2
$$848:
	.option	O2
	subu	$sp, 80
	sw	$31, 28($sp)
	sw	$18, 24($sp)
	sd	$16, 16($sp)
	.mask	0x80070000, -52
	.frame	$sp, 80, $31
	move	$16, $4
	sw	$5, 84($sp)
	.bgnb	1527
	.loc	2 2699
 #2694	    PixmapPtr pBackingPixmap;
 #2695	    miBSWindowPtr pBackingStore;
 #2696	    RegionPtr prgnSaved;
 #2697	    RegionPtr prgnRestored;
 #2698	    register ScreenPtr pScreen;
 #2699	    RegionPtr exposures = prgnExposed;
	lw	$14, 84($sp)
	sw	$14, 56($sp)
	.loc	2 2701
 #2700	
 #2701	    pScreen = pWin->drawable.pScreen;
	lw	$17, 16($16)
	.loc	2 2702
 #2702	    pBackingStore = (miBSWindowPtr)pWin->backStorage;
	lw	$15, 116($16)
	sw	$15, 72($sp)
	.loc	2 2703
 #2703	    pBackingPixmap = pBackingStore->pBackingPixmap;
	lw	$24, 72($sp)
	lw	$25, 0($24)
	sw	$25, 76($sp)
	.loc	2 2705
 #2704	
 #2705	    prgnSaved = &pBackingStore->SavedRegion;
	addu	$18, $24, 4
	.loc	2 2707
 #2706	
 #2707	    if (pBackingStore->status == StatusContents)
	lb	$2, 17($24)
	bne	$2, 5, $282
	.bgnb	1534
	.loc	2 2711
 #2708	    {
 #2709		miBSScreenPtr	pScreenPriv;
 #2710	
 #2711		(*pScreen->TranslateRegion) (prgnSaved, pWin->drawable.x, pWin->drawable.y);
	move	$4, $18
	lh	$5, 8($16)
	lh	$6, 10($16)
	lw	$8, 344($17)
	jal	$8
	.loc	2 2713
 #2712	
 #2713		prgnRestored = (* pScreen->RegionCreate)((BoxPtr)NULL, 1);
	move	$4, $0
	li	$5, 1
	lw	$9, 304($17)
	jal	$9
	sw	$2, 64($sp)
	.loc	2 2714
 #2714		(* pScreen->Intersect)(prgnRestored, prgnExposed, prgnSaved);
	move	$4, $2
	lw	$5, 84($sp)
	move	$6, $18
	lw	$10, 324($17)
	jal	$10
	.loc	2 2723
 #2723		(* pScreen->Subtract)(prgnSaved, prgnSaved, prgnExposed);
	move	$4, $18
	move	$5, $18
	lw	$6, 84($sp)
	lw	$11, 332($17)
	jal	$11
	lw	$4, 84($sp)
	.loc	2 2724
 #2724		(* pScreen->Subtract)(prgnExposed, prgnExposed, prgnRestored);
	move	$5, $4
	lw	$6, 64($sp)
	lw	$12, 332($17)
	jal	$12
	.loc	2 2731
 #2731		    pScreen->devPrivates[miBSScreenIndex].ptr;
	lw	$13, 404($17)
	lw	$14, $$839
	mul	$15, $14, 4
	addu	$25, $13, $15
	lw	$2, 0($25)
	.loc	2 2735
 #2732		(* pScreenPriv->funcs->RestoreAreas) (pBackingPixmap,
 #2733						 prgnRestored,
 #2734						 pWin->drawable.x,
 #2735						 pWin->drawable.y);
	lw	$4, 76($sp)
	lw	$5, 64($sp)
	lh	$6, 8($16)
	lh	$7, 10($16)
	lw	$24, 24($2)
	lw	$8, 4($24)
	jal	$8
	.loc	2 2737
 #2736		
 #2737		(* pScreen->RegionDestroy)(prgnRestored);
	lw	$4, 64($sp)
	lw	$9, 316($17)
	jal	$9
	.loc	2 2745
 #2745		if (!(*pScreen->RegionNotEmpty) (prgnSaved))
	move	$4, $18
	lw	$10, 356($17)
	jal	$10
	bne	$2, 0, $280
	.loc	2 2746
 #2746		    miDestroyBSPixmap (pWin);
	move	$4, $16
	jal	$$834
	b	$281
$280:
	.loc	2 2749
 #2747		else
 #2748		    (*pScreen->TranslateRegion) (prgnSaved,
 #2749						 -pWin->drawable.x, -pWin->drawable.y);
	move	$4, $18
	lh	$5, 8($16)
	negu	$5, $5
	lh	$6, 10($16)
	negu	$6, $6
	lw	$11, 344($17)
	jal	$11
$281:
	.loc	2 2750
 #2750	    }
	.endb	1536
	lw	$3, 124($16)
	sll	$3, $3, 10
	srl	$3, $3, 31
	b	$301
$282:
	.loc	2 2752
 #2751	    else if ((pBackingStore->status == StatusVirtual) ||
 #2752		     (pBackingStore->status == StatusVDirty))
	beq	$2, 2, $283
	bne	$2, 3, $290
$283:
	.loc	2 2755
 #2753	    {
 #2754		(*pScreen->TranslateRegion) (prgnSaved,
 #2755					     pWin->drawable.x, pWin->drawable.y);
	move	$4, $18
	lh	$5, 8($16)
	lh	$6, 10($16)
	lw	$12, 344($17)
	jal	$12
	.loc	2 2756
 #2756		exposures = (* pScreen->RegionCreate)(NullBox, 1);
	move	$4, $0
	li	$5, 1
	lw	$14, 304($17)
	jal	$14
	lw	$7, 72($sp)
	sw	$2, 56($sp)
	.loc	2 2760
 #2757		if (SameBackground (pBackingStore->backgroundState,
 #2758				    pBackingStore->background,
 #2759				    pWin->backgroundState,
 #2760	 			    pWin->background))
	lb	$3, 18($7)
	lw	$13, 124($16)
	and	$15, $13, 3
	bne	$15, $3, $287
	beq	$3, 0, $286
	beq	$3, 1, $286
	bne	$3, 2, $284
	lw	$25, 20($7)
	lw	$24, 108($16)
	seq	$6, $25, $24
	b	$285
$284:
	lw	$8, 20($7)
	lw	$9, 108($16)
	seq	$6, $8, $9
$285:
	beq	$6, 0, $287
$286:
	.loc	2 2762
 #2761		{
 #2762		    (* pScreen->Subtract)(exposures, prgnExposed, prgnSaved);
	lw	$4, 56($sp)
	lw	$5, 84($sp)
	move	$6, $18
	lw	$10, 332($17)
	jal	$10
	.loc	2 2763
 #2763		}
	b	$289
$287:
	.loc	2 2767
 #2764		else
 #2765		{
 #2766		    /* background has changed, virtually retile and expose */
 #2767		    if (pBackingStore->backgroundState == BackgroundPixmap)
	bne	$3, 3, $288
	.loc	2 2768
 #2768			(* pScreen->DestroyPixmap) (pBackingStore->background.pixmap);
	lw	$4, 20($7)
	lw	$11, 208($17)
	jal	$11
	lw	$7, 72($sp)
$288:
	.loc	2 2769
 #2769		    miTileVirtualBS(pWin);
	move	$4, $16
	jal	$$835
	.loc	2 2772
 #2770	
 #2771		    /* we need to expose all we have (virtually) retiled */
 #2772		    (* pScreen->Union) (exposures, prgnExposed, prgnSaved);
	lw	$4, 56($sp)
	lw	$5, 84($sp)
	move	$6, $18
	lw	$12, 328($17)
	jal	$12
	.loc	2 2773
 #2773		}
$289:
	.loc	2 2774
 #2774		(* pScreen->Subtract)(prgnSaved, prgnSaved, prgnExposed);
	move	$4, $18
	move	$5, $18
	lw	$6, 84($sp)
	lw	$14, 332($17)
	jal	$14
	.loc	2 2776
 #2775		(*pScreen->TranslateRegion) (prgnSaved,
 #2776					     -pWin->drawable.x, -pWin->drawable.y);
	move	$4, $18
	lh	$5, 8($16)
	negu	$5, $5
	lh	$6, 10($16)
	negu	$6, $6
	lw	$13, 344($17)
	jal	$13
	.loc	2 2777
 #2777	    }
	lw	$3, 124($16)
	sll	$3, $3, 10
	srl	$3, $3, 31
	b	$301
$290:
	.loc	2 2779
 #2778	    else if (pWin->viewable && !pBackingStore->viewable &&
 #2779		     pWin->backingStore != Always)
	lw	$2, 124($16)
	sll	$3, $2, 10
	srl	$3, $3, 31
	beq	$3, $0, $301
	lw	$15, 72($sp)
	lb	$25, 16($15)
	bne	$25, $0, $301
	sll	$24, $2, 26
	srl	$8, $24, 30
	beq	$8, 2, $301
	.bgnb	1537
	.loc	2 2790
 #2790		prgnSaved = &pBackingStore->SavedRegion;
	.loc	2 2792
 #2791	
 #2792		box.x1 = pWin->drawable.x;
	lh	$9, 8($16)
	sh	$9, 40($sp)
	.loc	2 2793
 #2793		box.x2 = box.x1 + (int) pWin->drawable.width;
	lh	$10, 40($sp)
	lhu	$11, 12($16)
	addu	$12, $10, $11
	sh	$12, 44($sp)
	.loc	2 2794
 #2794		box.y1 = pWin->drawable.y;
	lh	$14, 10($16)
	sh	$14, 42($sp)
	.loc	2 2795
 #2795		box.y2 = box.y1 + (int) pWin->drawable.height;
	lh	$13, 42($sp)
	lhu	$15, 14($16)
	addu	$25, $13, $15
	sh	$25, 46($sp)
	.loc	2 2797
 #2796		
 #2797		(* pScreen->Inverse)(prgnSaved, &pWin->clipList,  &box);
	move	$4, $18
	addu	$5, $16, 44
	addu	$6, $sp, 40
	lw	$24, 336($17)
	jal	$24
	.loc	2 2800
 #2798		(* pScreen->TranslateRegion) (prgnSaved,
 #2799					      -pWin->drawable.x,
 #2800					      -pWin->drawable.y);
	move	$4, $18
	lh	$5, 8($16)
	negu	$5, $5
	lh	$6, 10($16)
	negu	$6, $6
	lw	$8, 344($17)
	jal	$8
	.loc	2 2802
 #2801	#ifdef SHAPE
 #2802		if (wBoundingShape (pWin))
	lw	$2, 120($16)
	beq	$2, $0, $291
	lw	$6, 40($2)
	b	$292
$291:
	move	$6, $0
$292:
	beq	$6, 0, $295
	.loc	2 2803
 #2803		    (*pScreen->Intersect) (prgnSaved, prgnSaved, wBoundingShape (pWin));
	beq	$2, $0, $293
	lw	$6, 40($2)
	b	$294
$293:
	move	$6, $0
$294:
	move	$4, $18
	move	$5, $18
	lw	$9, 324($17)
	jal	$9
	lw	$2, 120($16)
$295:
	.loc	2 2804
 #2804		if (wClipShape (pWin))
	beq	$2, $0, $296
	lw	$6, 44($2)
	b	$297
$296:
	move	$6, $0
$297:
	beq	$6, 0, $300
	.loc	2 2805
 #2805		    (*pScreen->Intersect) (prgnSaved, prgnSaved, wClipShape (pWin));
	beq	$2, $0, $298
	lw	$6, 44($2)
	b	$299
$298:
	move	$6, $0
$299:
	move	$4, $18
	move	$5, $18
	lw	$10, 324($17)
	jal	$10
$300:
	.loc	2 2807
 #2806	#endif
 #2807		miTileVirtualBS(pWin);
	move	$4, $16
	jal	$$835
	.loc	2 2809
 #2808	
 #2809		exposures = (* pScreen->RegionCreate)(&box, 1);
	addu	$4, $sp, 40
	li	$5, 1
	lw	$11, 304($17)
	jal	$11
	sw	$2, 56($sp)
	.loc	2 2810
 #2810	    }
	.endb	1539
	lw	$3, 124($16)
	sll	$3, $3, 10
	srl	$3, $3, 31
$301:
	.loc	2 2811
 #2811	    pBackingStore->viewable = (char)pWin->viewable;
	lw	$12, 72($sp)
	sb	$3, 16($12)
	.loc	2 2812
 #2812	    return exposures;
	lw	$2, 56($sp)
	.endb	1540
	ld	$16, 16($sp)
	lw	$18, 24($sp)
	lw	$31, 28($sp)
	addu	$sp, 80
	j	$31
	.end	miBSRestoreAreas
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2837
 #2837	{
	.ent	$$850 2
$$850:
	.option	O2
	subu	$sp, 96
	sw	$31, 36($sp)
	sd	$16, 28($sp)
	.mask	0x80030000, -60
	.frame	$sp, 96, $31
	move	$16, $4
	sw	$5, 100($sp)
	sw	$6, 104($sp)
	sw	$7, 108($sp)
	.bgnb	1547
	.loc	2 2844
 #2844	    pScreen = pWin->drawable.pScreen;
	lw	$17, 16($16)
	.loc	2 2845
 #2845	    pBackingStore = (miBSWindowPtr)(pWin->backStorage);
	lw	$3, 116($16)
	.loc	2 2846
 #2846	    if (pBackingStore->status == StatusNoPixmap)
	lb	$2, 17($3)
	bne	$2, 1, $302
	.loc	2 2847
 #2847		return NullRegion;
	move	$2, $0
	b	$327
$302:
	.loc	2 2850
 #2848	
 #2849	    /* bit gravity makes things virtually too hard, punt */
 #2850	    if (((dx != 0) || (dy != 0)) && (pBackingStore->status != StatusContents))
	lw	$14, 100($sp)
	bne	$14, 0, $303
	lw	$15, 104($sp)
	beq	$15, 0, $304
$303:
	beq	$2, 5, $304
	.loc	2 2851
 #2851		miCreateBSPixmap(pWin);
	move	$4, $16
	sw	$3, 92($sp)
	jal	$$833
	lw	$3, 92($sp)
$304:
	.loc	2 2853
 #2852	
 #2853	    pSavedRegion = &pBackingStore->SavedRegion;
	addu	$24, $3, 4
	sw	$24, 44($sp)
	sw	$24, 88($sp)
	.loc	2 2854
 #2854	    if (!oldClip)
	lw	$25, 108($sp)
	bne	$25, 0, $305
	.loc	2 2855
 #2855		(* pScreen->RegionEmpty) (pSavedRegion);
	move	$4, $24
	lw	$8, 360($17)
	jal	$8
$305:
	.loc	2 2856
 #2856	    newSaved = (* pScreen->RegionCreate) (NullBox, 1);
	move	$4, $0
	li	$5, 1
	lw	$9, 304($17)
	jal	$9
	sw	$2, 84($sp)
	.loc	2 2857
 #2857	    exposed = (* pScreen->RegionCreate) (NullBox, 1);
	move	$4, $0
	li	$5, 1
	lw	$10, 304($17)
	jal	$10
	sw	$2, 80($sp)
	.loc	2 2859
 #2858	    /* resize and translate backing pixmap and SavedRegion */
 #2859	    miResizeBackingStore(pWin, dx, dy);
	move	$4, $16
	lw	$5, 100($sp)
	lw	$6, 104($sp)
	jal	$$1495
	.loc	2 2861
 #2860	    /* compute what the new pSavedRegion will be */
 #2861	    extents.x1 = pWin->drawable.x;
	lh	$11, 8($16)
	sh	$11, 68($sp)
	.loc	2 2862
 #2862	    extents.x2 = pWin->drawable.x + (int) pWin->drawable.width;
	lh	$12, 8($16)
	lhu	$13, 12($16)
	addu	$14, $12, $13
	sh	$14, 72($sp)
	.loc	2 2863
 #2863	    extents.y1 = pWin->drawable.y;
	lh	$15, 10($16)
	sh	$15, 70($sp)
	.loc	2 2864
 #2864	    extents.y2 = pWin->drawable.y + (int) pWin->drawable.height;
	lh	$25, 10($16)
	lhu	$24, 14($16)
	addu	$8, $25, $24
	sh	$8, 74($sp)
	.loc	2 2865
 #2865	    (* pScreen->Inverse)(newSaved, &pWin->clipList, &extents);
	lw	$4, 84($sp)
	addu	$5, $16, 44
	addu	$6, $sp, 68
	lw	$9, 336($17)
	jal	$9
	.loc	2 2867
 #2866	#ifdef SHAPE
 #2867	    if (wBoundingShape (pWin) || wClipShape (pWin)) {
	lw	$3, 120($16)
	beq	$3, $0, $306
	lw	$6, 40($3)
	b	$307
$306:
	move	$6, $0
$307:
	bne	$6, 0, $310
	beq	$3, $0, $308
	lw	$2, 44($3)
	b	$309
$308:
	move	$2, $0
$309:
	beq	$2, 0, $321
$310:
	.loc	2 2870
 #2868		(* pScreen->TranslateRegion) (newSaved,
 #2869					    -pWin->drawable.x,
 #2870					    -pWin->drawable.y);
	lw	$4, 84($sp)
	lh	$5, 8($16)
	negu	$5, $5
	lh	$6, 10($16)
	negu	$6, $6
	lw	$10, 344($17)
	jal	$10
	.loc	2 2871
 #2871		if (wBoundingShape (pWin))
	lw	$3, 120($16)
	beq	$3, $0, $311
	lw	$6, 40($3)
	b	$312
$311:
	move	$6, $0
$312:
	beq	$6, 0, $315
	.loc	2 2872
 #2872		    (* pScreen->Intersect) (newSaved, newSaved, wBoundingShape (pWin));
	beq	$3, $0, $313
	lw	$6, 40($3)
	b	$314
$313:
	move	$6, $0
$314:
	lw	$4, 84($sp)
	move	$5, $4
	lw	$11, 324($17)
	jal	$11
	lw	$3, 120($16)
$315:
	.loc	2 2873
 #2873		if (wClipShape (pWin))
	beq	$3, $0, $316
	lw	$6, 44($3)
	b	$317
$316:
	move	$6, $0
$317:
	beq	$6, 0, $320
	.loc	2 2874
 #2874		    (* pScreen->Intersect) (newSaved, newSaved, wClipShape (pWin));
	beq	$3, $0, $318
	lw	$6, 44($3)
	b	$319
$318:
	move	$6, $0
$319:
	lw	$4, 84($sp)
	move	$5, $4
	lw	$12, 324($17)
	jal	$12
$320:
	.loc	2 2877
 #2875		(* pScreen->TranslateRegion) (newSaved,
 #2876					    pWin->drawable.x,
 #2877					    pWin->drawable.y);
	lw	$4, 84($sp)
	lh	$5, 8($16)
	lh	$6, 10($16)
	lw	$13, 344($17)
	jal	$13
	.loc	2 2878
 #2878	    }
$321:
	.loc	2 2882
 #2879	#endif
 #2880	    
 #2881	    /* now find any visible areas we can save from the screen */
 #2882	    (* pScreen->TranslateRegion)(newSaved, -dx, -dy);
	lw	$4, 84($sp)
	lw	$5, 100($sp)
	negu	$5, $5
	lw	$6, 104($sp)
	negu	$6, $6
	lw	$14, 344($17)
	jal	$14
	.loc	2 2883
 #2883	    if (oldClip)
	lw	$15, 108($sp)
	beq	$15, 0, $322
	.loc	2 2885
 #2884	    {
 #2885		(* pScreen->Intersect) (exposed, oldClip, newSaved);
	lw	$4, 80($sp)
	move	$5, $15
	lw	$6, 84($sp)
	lw	$25, 324($17)
	jal	$25
	.loc	2 2886
 #2886		if ((* pScreen->RegionNotEmpty) (exposed))
	lw	$4, 80($sp)
	lw	$24, 356($17)
	jal	$24
	beq	$2, 0, $322
	.loc	2 2889
 #2887		{
 #2888		    /* save those visible areas */
 #2889		    (* pScreen->TranslateRegion) (exposed, dx, dy);
	lw	$4, 80($sp)
	lw	$5, 100($sp)
	lw	$6, 104($sp)
	lw	$8, 344($17)
	jal	$8
	.loc	2 2890
 #2890		    miBSSaveDoomedAreas(pWin, exposed, dx, dy);
	move	$4, $16
	lw	$5, 80($sp)
	lw	$6, 100($sp)
	lw	$7, 104($sp)
	jal	$$847
	.loc	2 2891
 #2891		}
	.loc	2 2892
 #2892	    }
$322:
	.loc	2 2896
 #2893	    /* translate newSaved to local coordinates */
 #2894	    (* pScreen->TranslateRegion) (newSaved,
 #2895					  dx-pWin->drawable.x,
 #2896					  dy-pWin->drawable.y);
	lw	$4, 84($sp)
	lw	$9, 100($sp)
	lh	$10, 8($16)
	subu	$5, $9, $10
	lw	$11, 104($sp)
	lh	$12, 10($16)
	subu	$6, $11, $12
	lw	$13, 344($17)
	jal	$13
	.loc	2 2898
 #2897	    /* subtract out what we already have saved */
 #2898	    (* pScreen->Subtract) (exposed, newSaved, pSavedRegion);
	lw	$4, 80($sp)
	lw	$5, 84($sp)
	lw	$6, 88($sp)
	lw	$14, 332($17)
	jal	$14
	.loc	2 2900
 #2899	    /* and expose whatever there is */
 #2900	    if ((* pScreen->RegionNotEmpty) (exposed))
	lw	$4, 80($sp)
	lw	$15, 356($17)
	jal	$15
	beq	$2, 0, $323
	.bgnb	1554
	.loc	2 2903
 #2901	    {
 #2902		RegionRec tmpRgn;
 #2903		extents = *((* pScreen->RegionExtents) (exposed));
	lw	$4, 80($sp)
	lw	$25, 364($17)
	jal	$25
	lw	$3, 44($sp)
	addu	$24, $sp, 68
	.set	 noat
	ulw	$1, 0($2)
	ulw	$8, 4($2)
	usw	$1, 0($24)
	usw	$8, 4($24)
	.set	 at
	.loc	2 2904
 #2904		tmpRgn = pBackingStore->SavedRegion; /* don't look */
	addu	$9, $sp, 48
	.set	 noat
	lw	$1, 0($3)
	lw	$10, 4($3)
	sw	$1, 0($9)
	lw	$1, 8($3)
	sw	$10, 4($9)
	sw	$1, 8($9)
	.set	 at
	.loc	2 2905
 #2905		pBackingStore->SavedRegion = *exposed;
	lw	$11, 80($sp)
	.set	 noat
	lw	$1, 0($11)
	lw	$12, 4($11)
	sw	$1, 0($3)
	lw	$1, 8($11)
	sw	$12, 4($3)
	sw	$1, 8($3)
	.set	 at
	.loc	2 2909
 #2906		(void) miBSClearBackingStore(pWin, extents.x1, extents.y1,
 #2907						extents.x2 - extents.x1,
 #2908						extents.y2 - extents.y1,
 #2909						FALSE);
	move	$4, $16
	lh	$13, 68($sp)
	move	$5, $13
	lh	$14, 70($sp)
	move	$6, $14
	lh	$15, 72($sp)
	subu	$7, $15, $13
	lh	$25, 74($sp)
	subu	$8, $25, $14
	sw	$8, 16($sp)
	sw	$0, 20($sp)
	jal	$$851
	.loc	2 2910
 #2910		pBackingStore->SavedRegion = tmpRgn;
	lw	$24, 44($sp)
	addu	$10, $sp, 48
	.set	 noat
	lw	$1, 0($10)
	lw	$9, 4($10)
	sw	$1, 0($24)
	lw	$1, 8($10)
	sw	$9, 4($24)
	sw	$1, 8($24)
	.set	 at
	.loc	2 2913
 #2911		(*pScreen->TranslateRegion) (exposed,
 #2912					     pWin->drawable.x,
 #2913					     pWin->drawable.y);
	lw	$4, 80($sp)
	lh	$5, 8($16)
	lh	$6, 10($16)
	lw	$12, 344($17)
	jal	$12
	.loc	2 2914
 #2914	    }
	.endb	1556
	b	$324
$323:
	.loc	2 2917
 #2915	    else
 #2916	    {
 #2917		(*pScreen->RegionDestroy) (exposed);
	lw	$4, 80($sp)
	lw	$11, 316($17)
	jal	$11
	.loc	2 2918
 #2918		exposed = NullRegion;
	sw	$0, 80($sp)
	.loc	2 2919
 #2919	    }
$324:
	.loc	2 2921
 #2920	    /* finally install new SavedRegion */
 #2921	    (* pScreen->RegionCopy) (pSavedRegion, newSaved);
	lw	$4, 88($sp)
	lw	$5, 84($sp)
	lw	$15, 312($17)
	jal	$15
	.loc	2 2922
 #2922	    (* pScreen->RegionDestroy) (newSaved);
	lw	$4, 84($sp)
	lw	$13, 316($17)
	jal	$13
	.loc	2 2927
 #2923	    /*
 #2924	     * an unrealized window will not get validate-tree'd, mash
 #2925	     * the serial number so GC's get revalidated for drawing
 #2926	     */
 #2927	    if (!pWin->realized)
	lw	$25, 124($16)
	sll	$14, $25, 11
	srl	$8, $14, 31
	bne	$8, $0, $326
	lw	$2, globalSerialNumber
	.loc	2 2928
 #2928		pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	addu	$2, $2, 1
	sw	$2, globalSerialNumber
	bleu	$2, 268435456, $325
	li	$9, 1
	sw	$9, globalSerialNumber
	li	$24, 1
	sw	$24, 20($16)
	b	$326
$325:
	lw	$10, globalSerialNumber
	sw	$10, 20($16)
$326:
	.loc	2 2929
 #2929	    return exposed;
	lw	$2, 80($sp)
	.endb	1557
$327:
	ld	$16, 28($sp)
	lw	$31, 36($sp)
	addu	$sp, 96
	j	$31
	.end	miBSTranslateBackingStore
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 2944
 #2944	{
	.ent	$$852 2
$$852:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	sw	$6, 32($sp)
	.bgnb	1563
	.loc	2 2947
 #2945	    miBSGCPtr 	pPriv;
 #2946	
 #2947	    if (pWin->backStorage)
	lw	$14, 116($4)
	beq	$14, $0, $331
	.loc	2 2949
 #2948	    {
 #2949		pPriv = (miBSGCPtr)pGC->devPrivates[miBSGCIndex].ptr;
	lw	$15, 76($5)
	lw	$24, $$853
	mul	$25, $24, 4
	addu	$8, $15, $25
	lw	$3, 0($8)
	.loc	2 2950
 #2950		if (!pPriv)
	bne	$3, 0, $328
	.loc	2 2951
 #2951		    miBSCreateGCPrivate (pGC);
	move	$4, $5
	sw	$5, 28($sp)
	jal	$$838
	lw	$5, 28($sp)
	lw	$9, 76($5)
	lw	$10, $$853
	mul	$11, $10, 4
	addu	$12, $9, $11
	lw	$3, 0($12)
$328:
	.loc	2 2952
 #2952		pPriv = (miBSGCPtr)pGC->devPrivates[miBSGCIndex].ptr;
	.loc	2 2953
 #2953		if (pPriv)
	beq	$3, 0, $331
	.loc	2 2963
 #2963		    switch (pPriv->guarantee)
	lw	$2, 4($3)
	b	$330
$329:
	.loc	2 2967
 #2964		    {
 #2965		    case GuaranteeNothing:
 #2966		    case GuaranteeVisBack:
 #2967			pPriv->guarantee = guarantee;
	lw	$13, 32($sp)
	sw	$13, 4($3)
	.loc	2 2968
 #2968			break;
	b	$331
$330:
	beq	$2, 0, $329
	beq	$2, 1, $329
	.loc	2 2970
 #2969		    }
 #2970		}
	.loc	2 2971
 #2971	    }
	.loc	2 2972
 #2972	}
	.endb	1565
$331:
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSDrawGuarantee
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3002
 #3002	{
	.ent	$$854 2
$$854:
	.option	O2
	subu	$sp, 80
	sw	$31, 20($sp)
	sw	$16, 16($sp)
	.mask	0x80010000, -60
	.frame	$sp, 80, $31
	move	$16, $4
	sw	$5, 84($sp)
	.bgnb	1571
	.loc	2 3008
 #3003	    GCPtr   	  	pBackingGC;
 #3004	    miBSWindowPtr	pWindowPriv;
 #3005	    miBSGCPtr		pPriv;
 #3006	    WindowPtr		pWin;
 #3007	    int			lift_functions;
 #3008	    RegionPtr		backingCompositeClip = NULL;
	sw	$0, 56($sp)
	.loc	2 3010
 #3009	
 #3010	    if (pDrawable->type == DRAWABLE_WINDOW)
	lbu	$14, 0($6)
	bne	$14, 0, $332
	.loc	2 3012
 #3011	    {
 #3012	        pWin = (WindowPtr) pDrawable;
	sw	$6, 64($sp)
	.loc	2 3013
 #3013		pWindowPriv = (miBSWindowPtr) pWin->backStorage;
	lw	$15, 116($6)
	sw	$15, 72($sp)
	.loc	2 3014
 #3014		lift_functions = (pWindowPriv == (miBSWindowPtr) NULL);
	lw	$24, 72($sp)
	seq	$25, $24, 0
	sw	$25, 60($sp)
	.loc	2 3015
 #3015	    }
	b	$333
$332:
	.loc	2 3018
 #3016	    else
 #3017	    {
 #3018	        pWin = (WindowPtr) NULL;
	sw	$0, 64($sp)
	.loc	2 3019
 #3019		lift_functions = TRUE;
	li	$8, 1
	sw	$8, 60($sp)
	.loc	2 3020
 #3020	    }
$333:
	.loc	2 3022
 #3021	
 #3022	    pPriv = (miBSGCPtr)pGC->devPrivates[miBSGCIndex].ptr;
	lw	$9, 76($16)
	lw	$10, $$853
	mul	$11, $10, 4
	addu	$12, $9, $11
	lw	$3, 0($12)
	.loc	2 3024
 #3023	
 #3024	    FUNC_PROLOGUE (pGC, pPriv);
	lw	$13, 20($3)
	sw	$13, 68($16)
	lw	$2, 16($3)
	sw	$2, 72($16)
	.loc	2 3026
 #3025	
 #3026	    (*pGC->funcs->ValidateGC) (pGC, stateChanges, pDrawable);
	move	$4, $16
	lw	$5, 84($sp)
	sw	$3, 68($sp)
	sw	$6, 88($sp)
	lw	$14, 68($16)
	lw	$15, 0($14)
	jal	$15
	lw	$3, 68($sp)
	.loc	2 3032
 #3027	
 #3028	    /*
 #3029	     * rewrap funcs and ops as Validate may have changed them
 #3030	     */
 #3031	
 #3032	    pPriv->wrapFuncs = pGC->funcs;
	lw	$24, 68($16)
	sw	$24, 20($3)
	.loc	2 3033
 #3033	    pPriv->wrapOps = pGC->ops;
	lw	$25, 72($16)
	sw	$25, 16($3)
	.loc	2 3035
 #3034	
 #3035	    if (pPriv->guarantee == GuaranteeVisBack)
	lw	$8, 4($3)
	bne	$8, 1, $334
	.loc	2 3036
 #3036	        lift_functions = TRUE;
	li	$10, 1
	sw	$10, 60($sp)
$334:
	.loc	2 3045
 #3045		 (stateChanges&(GCClipXOrigin|GCClipYOrigin|GCClipMask|GCSubwindowMode))))
	lw	$9, 60($sp)
	bne	$9, 0, $344
	lw	$11, 88($sp)
	lw	$12, 20($11)
	lw	$13, 8($3)
	bne	$12, $13, $335
	lw	$14, 84($sp)
	and	$15, $14, 950272
	beq	$15, $0, $344
$335:
	.loc	2 3047
 #3046	    {
 #3047		if ((*pGC->pScreen->RegionNotEmpty) (&pWindowPriv->SavedRegion))
	lw	$24, 72($sp)
	addu	$25, $24, 4
	sw	$25, 32($sp)
	move	$4, $25
	lw	$8, 0($16)
	lw	$10, 356($8)
	jal	$10
	beq	$2, 0, $340
	.loc	2 3049
 #3048	 	{
 #3049		    backingCompositeClip = (*pGC->pScreen->RegionCreate) (NULL, 1);
	move	$4, $0
	li	$5, 1
	lw	$9, 0($16)
	lw	$11, 304($9)
	jal	$11
	move	$4, $2
	.loc	2 3051
 #3050		    if ((pGC->clientClipType == CT_NONE) || 
 #3051			(pGC->clientClipType == CT_PIXMAP))
	lw	$3, 16($16)
	sll	$3, $3, 18
	srl	$3, $3, 30
	beq	$3, 0, $336
	bne	$3, 1, $337
$336:
	.loc	2 3054
 #3052		    {
 #3053			(*pGC->pScreen->RegionCopy) (backingCompositeClip,
 #3054						     &pWindowPriv->SavedRegion); 
	lw	$5, 32($sp)
	sw	$4, 56($sp)
	lw	$12, 0($16)
	lw	$13, 312($12)
	jal	$13
	.loc	2 3055
 #3055		    }
	b	$338
$337:
	.loc	2 3063
 #3063			(*pGC->pScreen->RegionCopy) (backingCompositeClip, pGC->clientClip);
	lw	$5, 56($16)
	sw	$4, 56($sp)
	lw	$14, 0($16)
	lw	$15, 312($14)
	jal	$15
	.loc	2 3066
 #3064			(*pGC->pScreen->TranslateRegion) (backingCompositeClip,
 #3065							  pGC->clipOrg.x,
 #3066							  pGC->clipOrg.y);
	lw	$4, 56($sp)
	lh	$5, 48($16)
	lh	$6, 50($16)
	lw	$24, 0($16)
	lw	$25, 344($24)
	jal	$25
	.loc	2 3068
 #3067			(*pGC->pScreen->Intersect) (backingCompositeClip, backingCompositeClip,
 #3068						    &pWindowPriv->SavedRegion);
	lw	$8, 56($sp)
	move	$4, $8
	move	$5, $8
	lw	$6, 32($sp)
	lw	$10, 0($16)
	lw	$9, 324($10)
	jal	$9
	.loc	2 3069
 #3069		    }
$338:
	.loc	2 3070
 #3070		    if (pGC->subWindowMode == IncludeInferiors)
	lw	$11, 16($16)
	sll	$12, $11, 21
	srl	$13, $12, 31
	bne	$13, 1, $339
	.bgnb	1578
	.loc	2 3079
 #3079			translatedClip = NotClippedByChildren (pWin);
	lw	$4, 64($sp)
	jal	NotClippedByChildren
	sw	$2, 48($sp)
	.loc	2 3082
 #3080			(*pGC->pScreen->TranslateRegion) (translatedClip,
 #3081							  pGC->clipOrg.x,
 #3082							  pGC->clipOrg.y);
	move	$4, $2
	lh	$5, 48($16)
	lh	$6, 50($16)
	lw	$14, 0($16)
	lw	$15, 344($14)
	jal	$15
	lw	$4, 56($sp)
	.loc	2 3083
 #3083			(*pGC->pScreen->Subtract) (backingCompositeClip, backingCompositeClip, translatedClip);
	move	$5, $4
	lw	$6, 48($sp)
	lw	$24, 0($16)
	lw	$25, 332($24)
	jal	$25
	.loc	2 3084
 #3084			(*pGC->pScreen->RegionDestroy) (translatedClip);
	lw	$4, 48($sp)
	lw	$8, 0($16)
	lw	$10, 316($8)
	jal	$10
	.loc	2 3085
 #3085		    }
	.endb	1580
$339:
	.loc	2 3087
 #3086	
 #3087		    if (!(*pGC->pScreen->RegionNotEmpty) (backingCompositeClip)) {
	lw	$4, 56($sp)
	lw	$9, 0($16)
	lw	$11, 356($9)
	jal	$11
	bne	$2, 0, $341
	.loc	2 3088
 #3088			lift_functions = TRUE;
	li	$12, 1
	sw	$12, 60($sp)
	.loc	2 3089
 #3089		    }
	.loc	2 3091
 #3090	
 #3091		}
	b	$341
$340:
	.loc	2 3094
 #3092	 	else
 #3093	 	{
 #3094		    lift_functions = TRUE;
	li	$13, 1
	sw	$13, 60($sp)
	.loc	2 3095
 #3095		}
$341:
	.loc	2 3104
 #3104		    (pWin->parent || pGC->subWindowMode != IncludeInferiors))
	lw	$14, 60($sp)
	beq	$14, 0, $343
	lw	$15, 72($sp)
	lb	$24, 17($15)
	bne	$24, 2, $343
	lw	$25, 64($sp)
	lw	$8, 24($25)
	bne	$8, $0, $342
	lw	$10, 16($16)
	sll	$9, $10, 21
	srl	$11, $9, 31
	beq	$11, 1, $343
$342:
	.loc	2 3105
 #3105		    pWindowPriv->status = StatusVDirty;
	li	$12, 3
	lw	$13, 72($sp)
	sb	$12, 17($13)
$343:
	.loc	2 3106
 #3106	    }
	lw	$3, 68($sp)
$344:
	.loc	2 3108
 #3107	
 #3108	    if (!lift_functions && !pWin->realized && pWin->backingStore != Always)
	lw	$14, 60($sp)
	bne	$14, 0, $345
	lw	$15, 64($sp)
	lw	$2, 124($15)
	sll	$24, $2, 11
	srl	$25, $24, 31
	bne	$25, $0, $345
	sll	$8, $2, 26
	srl	$10, $8, 30
	beq	$10, 2, $345
	.loc	2 3110
 #3109	    {
 #3110		lift_functions = TRUE;
	li	$9, 1
	sw	$9, 60($sp)
	.loc	2 3111
 #3111	    }
$345:
	.loc	2 3118
 #3118	    if (!lift_functions && !pWindowPriv->pBackingPixmap)
	lw	$11, 60($sp)
	bne	$11, 0, $347
	lw	$12, 72($sp)
	lw	$13, 0($12)
	bne	$13, $0, $347
	.loc	2 3120
 #3119	    {
 #3120		miCreateBSPixmap (pWin);
	lw	$4, 64($sp)
	jal	$$833
	.loc	2 3121
 #3121		if (!pWindowPriv->pBackingPixmap)
	lw	$14, 72($sp)
	lw	$15, 0($14)
	bne	$15, $0, $346
	.loc	2 3122
 #3122		    lift_functions = TRUE;
	li	$24, 1
	sw	$24, 60($sp)
$346:
	.loc	2 3123
 #3123	    }
	lw	$3, 68($sp)
$347:
	.loc	2 3130
 #3130	    if (!lift_functions && !pPriv->pBackingGC)
	lw	$25, 60($sp)
	bne	$25, 0, $350
	lw	$8, 0($3)
	bne	$8, $0, $350
	.bgnb	1581
	.loc	2 3135
 #3131	    {
 #3132		int status;
 #3133	
 #3134		pBackingGC = CreateGC ((DrawablePtr)pWindowPriv->pBackingPixmap,
 #3135				       (BITS32)0, (XID *)NULL, &status);
	lw	$10, 72($sp)
	lw	$4, 0($10)
	move	$5, $0
	move	$6, $0
	addu	$7, $sp, 44
	jal	CreateGC
	.loc	2 3136
 #3136		if (status != Success)
	lw	$9, 44($sp)
	beq	$9, 0, $348
	.loc	2 3137
 #3137		    lift_functions = TRUE;
	li	$11, 1
	sw	$11, 60($sp)
	b	$349
$348:
	.loc	2 3139
 #3138		else
 #3139		    pPriv->pBackingGC = pBackingGC;
	lw	$12, 68($sp)
	sw	$2, 0($12)
$349:
	.loc	2 3140
 #3140	    }
	.endb	1583
	lw	$3, 68($sp)
$350:
	.loc	2 3142
 #3141	
 #3142	    pBackingGC = pPriv->pBackingGC;
	lw	$5, 0($3)
	.loc	2 3144
 #3143	
 #3144	    pPriv->stateChanges |= stateChanges;
	lw	$13, 12($3)
	lw	$14, 84($sp)
	or	$15, $13, $14
	sw	$15, 12($3)
	.loc	2 3146
 #3145	
 #3146	    if (lift_functions)
	lw	$24, 60($sp)
	beq	$24, 0, $352
	lw	$4, 56($sp)
	.loc	2 3148
 #3147	    {
 #3148		if (backingCompositeClip)
	beq	$4, 0, $351
	.loc	2 3149
 #3149		    (* pGC->pScreen->RegionDestroy) (backingCompositeClip);
	lw	$25, 0($16)
	lw	$8, 316($25)
	jal	$8
$351:
	.loc	2 3152
 #3150	
 #3151		/* unwrap the GC again */
 #3152		miBSDestroyGCPrivate (pGC);
	move	$4, $16
	jal	$$1036
	.loc	2 3154
 #3153	
 #3154		return;
	b	$359
$352:
	.loc	2 3162
 #3162	    CopyGC(pGC, pBackingGC, pPriv->stateChanges);
	move	$4, $16
	lw	$6, 12($3)
	sw	$5, 76($sp)
	jal	CopyGC
	.loc	2 3164
 #3163	
 #3164	    pPriv->stateChanges = 0;
	lw	$10, 68($sp)
	sw	$0, 12($10)
	.loc	2 3170
 #3165	
 #3166	    /*
 #3167	     * We never want operations with the backingGC to generate GraphicsExpose
 #3168	     * events...
 #3169	     */
 #3170	    if (stateChanges & GCGraphicsExposures)
	lw	$9, 84($sp)
	and	$11, $9, 65536
	beq	$11, $0, $353
	.bgnb	1584
	.loc	2 3172
 #3171	    {
 #3172		XID false = xFalse;
	sw	$0, 40($sp)
	.loc	2 3174
 #3173	
 #3174		DoChangeGC(pBackingGC, GCGraphicsExposures, &false, FALSE);
	lw	$4, 76($sp)
	li	$5, 65536
	addu	$6, $sp, 40
	move	$7, $0
	jal	DoChangeGC
	.loc	2 3175
 #3175	    }
	.endb	1586
$353:
	lw	$6, 56($sp)
	.loc	2 3177
 #3176	
 #3177	    if (backingCompositeClip)
	beq	$6, 0, $356
	.loc	2 3179
 #3178	    {
 #3179		if (pGC->clientClipType == CT_PIXMAP)
	lw	$12, 16($16)
	sll	$13, $12, 18
	srl	$14, $13, 30
	bne	$14, 1, $354
	.bgnb	1587
	.loc	2 3184
 #3180		{
 #3181		    miBSScreenPtr   pScreenPriv;
 #3182	
 #3183		    pScreenPriv = (miBSScreenPtr) 
 #3184			pGC->pScreen->devPrivates[miBSScreenIndex].ptr;
	lw	$15, 0($16)
	lw	$24, 404($15)
	lw	$25, $$839
	mul	$8, $25, 4
	addu	$10, $24, $8
	lw	$2, 0($10)
	.loc	2 3186
 #3185		    (* pScreenPriv->funcs->SetClipmaskRgn)
 #3186			(pBackingGC, backingCompositeClip);
	lw	$4, 76($sp)
	move	$5, $6
	lw	$9, 24($2)
	lw	$11, 8($9)
	jal	$11
	.loc	2 3187
 #3187		    (* pGC->pScreen->RegionDestroy) (backingCompositeClip);
	lw	$4, 56($sp)
	lw	$12, 0($16)
	lw	$13, 316($12)
	jal	$13
	.loc	2 3188
 #3188		}
	.endb	1589
	b	$355
$354:
	.loc	2 3191
 #3189		else
 #3190		{
 #3191		    (*pBackingGC->funcs->ChangeClip) (pBackingGC, CT_REGION, backingCompositeClip, 0);
	lw	$14, 76($sp)
	move	$4, $14
	li	$5, 2
	move	$7, $0
	lw	$15, 68($14)
	lw	$25, 16($15)
	jal	$25
	.loc	2 3192
 #3192		}
$355:
	.loc	2 3193
 #3193		pPriv->serialNumber = pDrawable->serialNumber;
	lw	$24, 88($sp)
	lw	$8, 20($24)
	lw	$10, 68($sp)
	sw	$8, 8($10)
	.loc	2 3194
 #3194	    }
$356:
	lw	$5, 76($sp)
	.loc	2 3197
 #3195	    
 #3196	    if (pWindowPriv->pBackingPixmap->drawable.serialNumber
 #3197	    	!= pBackingGC->serialNumber)
	lw	$9, 72($sp)
	lw	$4, 0($9)
	lw	$11, 20($4)
	lw	$12, 64($5)
	beq	$11, $12, $357
	.loc	2 3199
 #3198	    {
 #3199		ValidateGC((DrawablePtr)pWindowPriv->pBackingPixmap, pBackingGC);
	jal	ValidateGC
	.loc	2 3200
 #3200	    }
	lw	$5, 76($sp)
$357:
	.loc	2 3202
 #3201	
 #3202	    if (pBackingGC->clientClip == 0)
	lw	$13, 56($5)
	bne	$13, 0, $358
	.loc	2 3203
 #3203	    	ErrorF ("backing store clip list nil");
	la	$4, $$1590
	jal	ErrorF
$358:
	.loc	2 3205
 #3204	
 #3205	    FUNC_EPILOGUE (pGC, pPriv);
	la	$14, $$861
	sw	$14, 68($16)
	la	$15, $$883
	sw	$15, 72($16)
	.loc	2 3206
 #3206	}
	.endb	1591
$359:
	lw	$16, 16($sp)
	lw	$31, 20($sp)
	addu	$sp, 80
	j	$31
	.end	miBSValidateGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3212
 #3207	
 #3208	static void
 #3209	miBSChangeGC (pGC, mask)
 #3210	    GCPtr   pGC;
 #3211	    unsigned long   mask;
 #3212	{
	.ent	$$857 2
$$857:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1596
	.loc	2 3213
 #3213	    miBSGCPtr	pPriv = (miBSGCPtr) (pGC)->devPrivates[miBSGCIndex].ptr;
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$2, 0($25)
	.loc	2 3215
 #3214	
 #3215	    FUNC_PROLOGUE (pGC, pPriv);
	lw	$8, 20($2)
	sw	$8, 68($4)
	lw	$3, 16($2)
	sw	$3, 72($4)
	.loc	2 3217
 #3216	
 #3217	    (*pGC->funcs->ChangeGC) (pGC, mask);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 4($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 3219
 #3218	
 #3219	    FUNC_EPILOGUE (pGC, pPriv);
	la	$11, $$861
	sw	$11, 68($4)
	la	$12, $$883
	sw	$12, 72($4)
	.loc	2 3220
 #3220	}
	.endb	1598
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSChangeGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3226
 #3221	
 #3222	static void
 #3223	miBSCopyGC (pGCSrc, mask, pGCDst)
 #3224	    GCPtr   pGCSrc, pGCDst;
 #3225	    unsigned long   mask;
 #3226	{
	.ent	$$855 2
$$855:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1604
	.loc	2 3227
 #3227	    miBSGCPtr	pPriv = (miBSGCPtr) (pGCDst)->devPrivates[miBSGCIndex].ptr;
	lw	$14, 76($6)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$2, 0($25)
	.loc	2 3229
 #3228	
 #3229	    FUNC_PROLOGUE (pGCDst, pPriv);
	lw	$8, 20($2)
	sw	$8, 68($6)
	lw	$3, 16($2)
	sw	$3, 72($6)
	.loc	2 3231
 #3230	
 #3231	    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
	sw	$6, 32($sp)
	lw	$9, 68($6)
	lw	$10, 8($9)
	jal	$10
	lw	$6, 32($sp)
	.loc	2 3233
 #3232	
 #3233	    FUNC_EPILOGUE (pGCDst, pPriv);
	la	$11, $$861
	sw	$11, 68($6)
	la	$12, $$883
	sw	$12, 72($6)
	.loc	2 3234
 #3234	}
	.endb	1606
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCopyGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3239
 #3235	
 #3236	static void
 #3237	miBSDestroyGC (pGC)
 #3238	    GCPtr   pGC;
 #3239	{
	.ent	$$856 2
$$856:
	.option	O2
	subu	$sp, 32
	sw	$31, 20($sp)
	.mask	0x80000000, -12
	.frame	$sp, 32, $31
	move	$3, $4
	.bgnb	1610
	.loc	2 3240
 #3240	    miBSGCPtr	pPriv = (miBSGCPtr) (pGC)->devPrivates[miBSGCIndex].ptr;
	lw	$14, 76($3)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$6, 0($25)
	.loc	2 3242
 #3241	
 #3242	    FUNC_PROLOGUE (pGC, pPriv);
	lw	$8, 20($6)
	sw	$8, 68($3)
	lw	$2, 16($6)
	sw	$2, 72($3)
	.loc	2 3244
 #3243	
 #3244	    if (pPriv->pBackingGC)
	lw	$7, 0($6)
	beq	$7, $0, $360
	.loc	2 3245
 #3245		FreeGC(pPriv->pBackingGC, (GContext)0);
	move	$4, $7
	move	$5, $0
	sw	$3, 32($sp)
	sw	$6, 28($sp)
	jal	FreeGC
	lw	$3, 32($sp)
	lw	$6, 28($sp)
$360:
	.loc	2 3247
 #3246	
 #3247	    (*pGC->funcs->DestroyGC) (pGC);
	move	$4, $3
	sw	$3, 32($sp)
	sw	$6, 28($sp)
	lw	$9, 68($3)
	lw	$10, 12($9)
	jal	$10
	lw	$3, 32($sp)
	lw	$6, 28($sp)
	.loc	2 3249
 #3248	
 #3249	    FUNC_EPILOGUE (pGC, pPriv);
	la	$11, $$861
	sw	$11, 68($3)
	la	$12, $$883
	sw	$12, 72($3)
	.loc	2 3251
 #3250	
 #3251	    xfree(pPriv);
	move	$4, $6
	jal	Xfree
	.loc	2 3252
 #3252	}
	.endb	1612
	lw	$31, 20($sp)
	addu	$sp, 32
	j	$31
	.end	miBSDestroyGC
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3260
 #3260	{
	.ent	$$858 2
$$858:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1619
	.loc	2 3261
 #3261	    miBSGCPtr	pPriv = (miBSGCPtr) (pGC)->devPrivates[miBSGCIndex].ptr;
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$2, 0($25)
	.loc	2 3263
 #3262	
 #3263	    FUNC_PROLOGUE (pGC, pPriv);
	lw	$8, 20($2)
	sw	$8, 68($4)
	lw	$3, 16($2)
	sw	$3, 72($4)
	.loc	2 3265
 #3264	
 #3265	    (* pGC->funcs->ChangeClip)(pGC, type, pvalue, nrects);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 16($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 3267
 #3266	
 #3267	    FUNC_EPILOGUE (pGC, pPriv);
	la	$11, $$861
	sw	$11, 68($4)
	la	$12, $$883
	sw	$12, 72($4)
	.loc	2 3268
 #3268	}
	.endb	1621
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSChangeClip
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3273
 #3269	
 #3270	static void
 #3271	miBSCopyClip(pgcDst, pgcSrc)
 #3272	    GCPtr pgcDst, pgcSrc;
 #3273	{
	.ent	$$860 2
$$860:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1626
	.loc	2 3274
 #3274	    miBSGCPtr	pPriv = (miBSGCPtr) (pgcDst)->devPrivates[miBSGCIndex].ptr;
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$2, 0($25)
	.loc	2 3276
 #3275	
 #3276	    FUNC_PROLOGUE (pgcDst, pPriv);
	lw	$8, 20($2)
	sw	$8, 68($4)
	lw	$3, 16($2)
	sw	$3, 72($4)
	.loc	2 3278
 #3277	
 #3278	    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 24($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 3280
 #3279	
 #3280	    FUNC_EPILOGUE (pgcDst, pPriv);
	la	$11, $$861
	sw	$11, 68($4)
	la	$12, $$883
	sw	$12, 72($4)
	.loc	2 3281
 #3281	}
	.endb	1628
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSCopyClip
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3286
 #3282	
 #3283	static void
 #3284	miBSDestroyClip(pGC)
 #3285	    GCPtr	pGC;
 #3286	{
	.ent	$$859 2
$$859:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	.bgnb	1632
	.loc	2 3287
 #3287	    miBSGCPtr	pPriv = (miBSGCPtr) (pGC)->devPrivates[miBSGCIndex].ptr;
	lw	$14, 76($4)
	lw	$15, $$853
	mul	$24, $15, 4
	addu	$25, $14, $24
	lw	$2, 0($25)
	.loc	2 3289
 #3288	
 #3289	    FUNC_PROLOGUE (pGC, pPriv);
	lw	$8, 20($2)
	sw	$8, 68($4)
	lw	$3, 16($2)
	sw	$3, 72($4)
	.loc	2 3291
 #3290	
 #3291	    (* pGC->funcs->DestroyClip)(pGC);
	sw	$4, 24($sp)
	lw	$9, 68($4)
	lw	$10, 20($9)
	jal	$10
	lw	$4, 24($sp)
	.loc	2 3293
 #3292	
 #3293	    FUNC_EPILOGUE (pGC, pPriv);
	la	$11, $$861
	sw	$11, 68($4)
	la	$12, $$883
	sw	$12, 72($4)
	.loc	2 3294
 #3294	}
	.endb	1634
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miBSDestroyClip
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3299
 #3295	
 #3296	static void
 #3297	miDestroyBSPixmap (pWin)
 #3298	    WindowPtr	pWin;
 #3299	{
	.ent	$$834 2
$$834:
	.option	O2
	subu	$sp, 32
	sw	$31, 20($sp)
	.mask	0x80000000, -12
	.frame	$sp, 32, $31
	sw	$4, 32($sp)
	.bgnb	1638
	.loc	2 3303
 #3300	    miBSWindowPtr	pBackingStore;
 #3301	    ScreenPtr		pScreen;
 #3302	    
 #3303	    pScreen = pWin->drawable.pScreen;
	lw	$14, 32($sp)
	lw	$5, 16($14)
	.loc	2 3304
 #3304	    pBackingStore = (miBSWindowPtr) pWin->backStorage;
	lw	$2, 116($14)
	.loc	2 3305
 #3305	    if (pBackingStore->pBackingPixmap)
	lw	$3, 0($2)
	beq	$3, $0, $361
	.loc	2 3306
 #3306		(* pScreen->DestroyPixmap)(pBackingStore->pBackingPixmap);
	move	$4, $3
	sw	$2, 28($sp)
	sw	$5, 24($sp)
	lw	$15, 208($5)
	jal	$15
	lw	$2, 28($sp)
	lw	$5, 24($sp)
$361:
	.loc	2 3307
 #3307	    pBackingStore->pBackingPixmap = NullPixmap;
	sw	$0, 0($2)
	.loc	2 3308
 #3308	    if (pBackingStore->backgroundState == BackgroundPixmap)
	lb	$24, 18($2)
	bne	$24, 3, $362
	.loc	2 3309
 #3309		(* pScreen->DestroyPixmap)(pBackingStore->background.pixmap);
	lw	$4, 20($2)
	sw	$2, 28($sp)
	lw	$25, 208($5)
	jal	$25
	lw	$2, 28($sp)
$362:
	.loc	2 3310
 #3310	    pBackingStore->backgroundState = None;
	sb	$0, 18($2)
	.loc	2 3311
 #3311	    pBackingStore->status = StatusNoPixmap;
	li	$8, 1
	sb	$8, 17($2)
	.loc	2 3312
 #3312	    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	lw	$9, globalSerialNumber
	addu	$10, $9, 1
	sw	$10, globalSerialNumber
	bleu	$10, 268435456, $363
	li	$11, 1
	sw	$11, globalSerialNumber
	li	$12, 1
	lw	$13, 32($sp)
	sw	$12, 20($13)
	b	$364
$363:
	lw	$14, globalSerialNumber
	lw	$15, 32($sp)
	sw	$14, 20($15)
	.loc	2 3313
 #3313	}
	.endb	1641
$364:
	lw	$31, 20($sp)
	addu	$sp, 32
	j	$31
	.end	miDestroyBSPixmap
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3318
 #3314	
 #3315	static void
 #3316	miTileVirtualBS (pWin)
 #3317	    WindowPtr	pWin;
 #3318	{
	.ent	$$835 2
$$835:
	.option	O2
	subu	$sp, 24
	sw	$31, 20($sp)
	.mask	0x80000000, -4
	.frame	$sp, 24, $31
	li	$5, 3
	.bgnb	1645
	.loc	2 3321
 #3319	    miBSWindowPtr	pBackingStore;
 #3320	
 #3321	    pBackingStore = (miBSWindowPtr) pWin->backStorage;
	lw	$3, 116($4)
	.loc	2 3322
 #3322	    pBackingStore->backgroundState = pWin->backgroundState;
	lw	$14, 124($4)
	and	$15, $14, 3
	sb	$15, 18($3)
	.loc	2 3323
 #3323	    pBackingStore->background = pWin->background;
	.set	 noat
	lw	$1, 108($4)
	sw	$1, 20($3)
	.set	 at
	.loc	2 3324
 #3324	    if (pBackingStore->backgroundState == BackgroundPixmap)
	lb	$25, 18($3)
	bne	$25, $5, $365
	.loc	2 3325
 #3325		pBackingStore->background.pixmap->refcnt++;
	lw	$2, 20($3)
	lw	$8, 24($2)
	addu	$9, $8, 1
	sw	$9, 24($2)
$365:
	.loc	2 3327
 #3326	
 #3327	    if (pBackingStore->status != StatusVDirty)
	lb	$10, 17($3)
	beq	$10, $5, $366
	.loc	2 3328
 #3328		pBackingStore->status = StatusVirtual;
	li	$11, 2
	sb	$11, 17($3)
$366:
	.loc	2 3333
 #3329	
 #3330	    /*
 #3331	     * punt parent relative tiles and do it now
 #3332	     */
 #3333	    if (pBackingStore->backgroundState == ParentRelative)
	lb	$12, 18($3)
	bne	$12, 1, $367
	.loc	2 3334
 #3334		miCreateBSPixmap (pWin);
	jal	$$833
	.loc	2 3335
 #3335	}
	.endb	1647
$367:
	lw	$31, 20($sp)
	addu	$sp, 24
	j	$31
	.end	miTileVirtualBS
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3347
 #3347	{
	.ent	$$833 2
$$833:
	.option	O2
	subu	$sp, 80
	sw	$31, 36($sp)
	sw	$18, 32($sp)
	sd	$16, 24($sp)
	.mask	0x80070000, -44
	.frame	$sp, 80, $31
	move	$16, $4
	.bgnb	1651
	.loc	2 3355
 #3355	    pScreen = pWin->drawable.pScreen;
	lw	$18, 16($16)
	.loc	2 3356
 #3356	    pBackingStore = (miBSWindowPtr) pWin->backStorage;
	lw	$17, 116($16)
	.loc	2 3358
 #3357	    backSet = ((pBackingStore->status == StatusVirtual) ||
 #3358		       (pBackingStore->status == StatusVDirty));
	lb	$3, 17($17)
	seq	$2, $3, 2
	bne	$2, 0, $368
	seq	$2, $3, 3
$368:
	sw	$2, 56($sp)
	.loc	2 3360
 #3359	
 #3360	    if (!pBackingStore->pBackingPixmap)
	lw	$3, 0($17)
	bne	$3, $0, $369
	.loc	2 3366
 #3361		pBackingStore->pBackingPixmap =
 #3362	    	    (PixmapPtr)(* pScreen->CreatePixmap)
 #3363				   (pScreen,
 #3364				    pWin->drawable.width,
 #3365				    pWin->drawable.height,
 #3366				    pWin->drawable.depth);
	move	$4, $18
	lhu	$5, 12($16)
	lhu	$6, 14($16)
	lbu	$7, 2($16)
	lw	$14, 204($18)
	jal	$14
	sw	$2, 0($17)
	lw	$3, 0($17)
$369:
	.loc	2 3368
 #3367	
 #3368	    if (!pBackingStore->pBackingPixmap)
	bne	$3, $0, $370
	.loc	2 3381
 #3381		pBackingStore->status = StatusNoPixmap;
	li	$15, 1
	sb	$15, 17($17)
	.loc	2 3382
 #3382		return; /* XXX */
	b	$375
$370:
	.loc	2 3385
 #3383	    }
 #3384	
 #3385	    pBackingStore->status = StatusContents;
	li	$24, 5
	sb	$24, 17($17)
	.loc	2 3387
 #3386	
 #3387	    if (backSet)
	lw	$25, 56($sp)
	beq	$25, 0, $371
	.loc	2 3389
 #3388	    {
 #3389		backgroundState = pWin->backgroundState;
	lw	$8, 124($16)
	and	$9, $8, 3
	sb	$9, 67($sp)
	.loc	2 3390
 #3390		background = pWin->background;
	addu	$3, $16, 108
	addu	$10, $sp, 68
	.set	 noat
	lw	$1, 0($3)
	sw	$1, 0($10)
	.set	 at
	.loc	2 3392
 #3391	    
 #3392		pWin->backgroundState = pBackingStore->backgroundState;
	lw	$2, 124($16)
	lb	$12, 18($17)
	xor	$13, $12, $2
	and	$14, $13, 3
	xor	$15, $14, $2
	sw	$15, 124($16)
	.loc	2 3393
 #3393		pWin->background = pBackingStore->background;
	.set	 noat
	lw	$1, 20($17)
	sw	$1, 0($3)
	.set	 at
	.loc	2 3394
 #3394		if (pWin->backgroundState == BackgroundPixmap)
	lw	$25, 124($16)
	and	$8, $25, 3
	bne	$8, 3, $371
	.loc	2 3395
 #3395		    pWin->background.pixmap->refcnt++;
	lw	$2, 108($16)
	lw	$9, 24($2)
	addu	$11, $9, 1
	sw	$11, 24($2)
	.loc	2 3396
 #3396	    }
$371:
	.loc	2 3398
 #3397	
 #3398	    if ((* pScreen->RegionNotEmpty) (&pBackingStore->SavedRegion))
	addu	$10, $17, 4
	sw	$10, 48($sp)
	move	$4, $10
	lw	$12, 356($18)
	jal	$12
	beq	$2, 0, $372
	.loc	2 3400
 #3399	    {
 #3400		extents = (* pScreen->RegionExtents) (&pBackingStore->SavedRegion);
	lw	$4, 48($sp)
	lw	$13, 364($18)
	jal	$13
	.loc	2 3405
 #3401		miBSClearBackingStore(pWin,
 #3402				      extents->x1, extents->y1,
 #3403				      extents->x2 - extents->x1,
 #3404				      extents->y2 - extents->y1,
 #3405				      FALSE);
	move	$4, $16
	lh	$5, 0($2)
	lh	$6, 2($2)
	lh	$14, 4($2)
	subu	$7, $14, $5
	lh	$15, 6($2)
	subu	$24, $15, $6
	sw	$24, 16($sp)
	sw	$0, 20($sp)
	jal	$$851
	.loc	2 3406
 #3406	    }
$372:
	.loc	2 3408
 #3407	
 #3408	    if (backSet)
	lw	$25, 56($sp)
	beq	$25, 0, $375
	.loc	2 3410
 #3409	    {
 #3410		if (pWin->backgroundState == BackgroundPixmap)
	lw	$2, 124($16)
	addu	$3, $16, 108
	and	$8, $2, 3
	bne	$8, 3, $373
	.loc	2 3411
 #3411		    (* pScreen->DestroyPixmap) (pWin->background.pixmap);
	lw	$4, 108($16)
	sw	$3, 44($sp)
	lw	$9, 208($18)
	jal	$9
	lw	$3, 44($sp)
	lw	$2, 124($16)
$373:
	.loc	2 3412
 #3412		pWin->backgroundState = backgroundState;
	lb	$11, 67($sp)
	xor	$10, $11, $2
	and	$12, $10, 3
	xor	$13, $12, $2
	sw	$13, 124($16)
	.loc	2 3413
 #3413		pWin->background = background;
	addu	$14, $sp, 68
	.set	 noat
	lw	$1, 0($14)
	sw	$1, 0($3)
	.set	 at
	.loc	2 3414
 #3414		if (pBackingStore->backgroundState == BackgroundPixmap)
	lb	$24, 18($17)
	bne	$24, 3, $374
	.loc	2 3415
 #3415		    (* pScreen->DestroyPixmap) (pBackingStore->background.pixmap);
	lw	$4, 20($17)
	lw	$25, 208($18)
	jal	$25
$374:
	.loc	2 3416
 #3416		pBackingStore->backgroundState = None;
	sb	$0, 18($17)
	.loc	2 3417
 #3417	    }
	.loc	2 3418
 #3418	}
	.endb	1658
$375:
	ld	$16, 24($sp)
	lw	$18, 32($sp)
	lw	$31, 36($sp)
	addu	$sp, 80
	j	$31
	.end	miCreateBSPixmap
	.text	
	.align	2
	.file	2 "mibstore.c"
	.loc	2 3443
 #3443	{
	.ent	$$849 2
$$849:
	.option	O2
	subu	$sp, 120
	sd	$30, 56($sp)
	sw	$19, 52($sp)
	sd	$16, 44($sp)
	.mask	0xC00B0000, -60
	.frame	$sp, 120, $31
	move	$17, $4
	move	$30, $5
	move	$19, $6
	move	$16, $7
	.bgnb	1670
	.loc	2 3451
 #3451	    if (!(*pGC->pScreen->RegionNotEmpty) (prgnExposed))
	move	$4, $16
	lw	$14, 0($19)
	lw	$15, 356($14)
	jal	$15
	beq	$2, 0, $388
	.loc	2 3452
 #3452		return;
	sw	$22, 80($sp)
	.loc	2 3453
 #3453	    pBackingStore = (miBSWindowPtr)pSrc->backStorage;
	lw	$22, 116($17)
	.loc	2 3455
 #3454	    
 #3455	    if (pBackingStore->status == StatusNoPixmap)
	lb	$24, 17($22)
	bne	$24, 1, $376
	.loc	2 3456
 #3456	    	return;
	lw	$22, 80($sp)
	b	$388
$376:
	.loc	2 3458
 #3457	
 #3458	    tempRgn = (* pGC->pScreen->RegionCreate) (NULL, 1);
	move	$4, $0
	li	$5, 1
	lw	$25, 0($19)
	lw	$8, 304($25)
	jal	$8
	sw	$18, 76($sp)
	sw	$20, 72($sp)
	sw	$21, 68($sp)
	sw	$23, 64($sp)
	sw	$2, 116($sp)
	.loc	2 3460
 #3459	    (* pGC->pScreen->Intersect) (tempRgn, prgnExposed,
 #3460					 &pBackingStore->SavedRegion);
	move	$4, $2
	move	$5, $16
	addu	$6, $22, 4
	lw	$9, 0($19)
	lw	$10, 324($9)
	jal	$10
	.loc	2 3461
 #3461	    (* pGC->pScreen->Subtract) (prgnExposed, prgnExposed, tempRgn);
	move	$4, $16
	move	$5, $16
	lw	$6, 116($sp)
	lw	$11, 0($19)
	lw	$12, 332($11)
	jal	$12
	lw	$23, 152($sp)
	.loc	2 3463
 #3462	
 #3463	    if (plane != 0) {
	beq	$23, 0, $377
	.loc	2 3464
 #3464		copyProc = pGC->ops->CopyPlane;
	lw	$13, 72($19)
	lw	$18, 16($13)
	.loc	2 3465
 #3465	    } else {
	b	$378
$377:
	.loc	2 3466
 #3466		copyProc = pGC->ops->CopyArea;
	lw	$14, 72($19)
	lw	$18, 12($14)
	.loc	2 3467
 #3467	    }
$378:
	.loc	2 3469
 #3468	    
 #3469	    dx = dstx - srcx;
	lw	$15, 144($sp)
	lw	$24, 136($sp)
	subu	$20, $15, $24
	.loc	2 3470
 #3470	    dy = dsty - srcy;
	lw	$25, 148($sp)
	lw	$8, 140($sp)
	subu	$21, $25, $8
	.loc	2 3472
 #3471	    
 #3472	    switch (pBackingStore->status) {
	lb	$2, 17($22)
	b	$386
$379:
	.loc	2 3475
 #3473	    case StatusVirtual:
 #3474	    case StatusVDirty:
 #3475		pGC = GetScratchGC (pDst->depth, pDst->pScreen);
	lbu	$4, 2($30)
	lw	$5, 16($30)
	jal	GetScratchGC
	move	$19, $2
	.loc	2 3476
 #3476		if (pGC)
	beq	$2, 0, $387
	.loc	2 3481
 #3477		{
 #3478		    miBSFillVirtualBits (pDst, pGC, tempRgn, dx, dy,
 #3479					 pBackingStore->backgroundState,
 #3480					 pBackingStore->background,
 #3481					 ~0L);
	move	$4, $30
	move	$5, $19
	lw	$6, 116($sp)
	move	$7, $20
	sw	$21, 16($sp)
	lb	$9, 18($22)
	sw	$9, 20($sp)
	.set	 noat
	lw	$1, 20($22)
	sw	$1, 24($sp)
	.set	 at
	li	$11, -1
	sw	$11, 28($sp)
	jal	$$907
	.loc	2 3482
 #3482		    FreeScratchGC (pGC);
	move	$4, $19
	jal	FreeScratchGC
	.loc	2 3483
 #3483		}
	.loc	2 3484
 #3484		break;
	b	$387
$380:
	lw	$3, 116($sp)
	.loc	2 3487
 #3485	    case StatusContents:
 #3486		for (i = REGION_NUM_RECTS(tempRgn), pBox = REGION_RECTS(tempRgn);
 #3487		     --i >= 0;
	lw	$2, 8($3)
	beq	$2, $0, $381
	lw	$17, 4($2)
	b	$382
$381:
	li	$17, 1
$382:
	beq	$2, $0, $383
	addu	$16, $2, 8
	b	$384
$383:
	move	$16, $3
$384:
	.loc	2 3487
	addu	$17, $17, -1
	blt	$17, 0, $387
$385:
	.loc	2 3493
 #3488		     pBox++)
 #3489		{
 #3490		    (* copyProc) (pBackingStore->pBackingPixmap,
 #3491				  pDst, pGC, pBox->x1, pBox->y1,
 #3492				  pBox->x2 - pBox->x1, pBox->y2 - pBox->y1,
 #3493				  pBox->x1 + dx, pBox->y1 + dy, plane);
	lw	$4, 0($22)
	move	$5, $30
	move	$6, $19
	lh	$7, 0($16)
	lh	$2, 2($16)
	sw	$2, 16($sp)
	lh	$12, 4($16)
	subu	$13, $12, $7
	sw	$13, 20($sp)
	lh	$14, 6($16)
	subu	$15, $14, $2
	sw	$15, 24($sp)
	addu	$24, $7, $20
	sw	$24, 28($sp)
	addu	$25, $2, $21
	sw	$25, 32($sp)
	sw	$23, 36($sp)
	jal	$18
	.loc	2 3494
 #3494		}
	.loc	2 3494
	addu	$16, $16, 8
	.loc	2 3494
	addu	$17, $17, -1
	bge	$17, 0, $385
	.loc	2 3495
 #3495		break;
	b	$387
$386:
	beq	$2, 2, $379
	beq	$2, 3, $379
	beq	$2, 5, $380
$387:
	.loc	2 3497
 #3496	    }
 #3497	}
	.endb	1678
	lw	$18, 76($sp)
	lw	$20, 72($sp)
	lw	$21, 68($sp)
	lw	$22, 80($sp)
	lw	$23, 64($sp)
$388:
	ld	$16, 44($sp)
	lw	$19, 52($sp)
	ld	$30, 56($sp)
	addu	$sp, 120
	j	$31
	.end	miBSExposeCopy
