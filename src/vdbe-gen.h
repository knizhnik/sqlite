static Mem *out2Prerelease(Vdbe *p, int p2){
  Mem *pOut;
  assert( p2>0 );
  assert( p2<=(p->nMem-p->nCursor) );
  pOut = &p->aMem[p2];
  memAboutToChange(p, pOut);
  if( VdbeMemDynamic(pOut) ){
    return out2PrereleaseWithClear(pOut);
  }else{
    pOut->flags = MEM_Int;
    return pOut;
  }
}
