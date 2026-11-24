# Writes mon_<res>.bin: GEOS 1 VisMoniker (bitmap gstring) for each icon size,
# same layout as the working Schnecke icon (type, size, GR 0x1a, PackBits).
import struct, make_icon as m
SPEC=[(5,'c64x40',0x46),(6,'m64x40',0x41),(7,'c48x30',0x46),(8,'m48x30',0x41),(9,'m64x18',0x61),(10,'m48x14',0x61)]
def packbits(row):
    out=bytearray(); i=0; n=len(row)
    while i<n:
        j=i
        while j+1<n and row[j+1]==row[i] and j-i<127: j+=1
        run=j-i+1
        if run>=3:
            out+=bytes([257-run,row[i]]); i=j+1; continue
        lit=bytearray()
        while i<n and len(lit)<128:
            if i+2<n and row[i]==row[i+1]==row[i+2]: break
            lit.append(row[i]); i+=1
        out+=bytes([len(lit)-1])+lit
    return bytes(out)
def unpack(data,nbytes):
    out=bytearray(); i=0
    while len(out)<nbytes:
        c=data[i]; i+=1
        if c<128: out+=data[i:i+c+1]; i+=c+1
        elif c==128: pass
        else: out+=bytes([data[i]])*(257-c); i+=1
    return bytes(out)
for res,name,vmtype in SPEC:
    w,h,color,img=m.out[name]
    data=bytearray(); rawall=bytearray()
    for y in range(h):
        row=bytearray()
        if color:
            mask=bytearray((w+7)//8); pix=bytearray((w+1)//2)
            for x in range(w):
                v=img[y][x]
                if v!=m.T:
                    mask[x//8]|=0x80>>(x%8)
                    pix[x//2]|=(v<<4) if x%2==0 else v
                else:
                    pix[x//2]|=(15<<4) if x%2==0 else 15
            row=mask+pix
        else:
            row=bytearray((w+7)//8)
            for x in range(w):
                if img[y][x]==0: row[x//8]|=0x80>>(x%8)   # 1 = black
        rawall+=row; data+=packbits(bytes(row))
    assert unpack(bytes(data),len(rawall))==bytes(rawall)
    btype=0x11 if color else 0x00
    bmp=struct.pack('<HHBB',w,h,1,btype)+data
    mon=struct.pack('<BHH',vmtype,w,h)+bytes([0x1a])+struct.pack('<H',len(bmp))+bmp+b'\0'
    open('mon_%d.bin'%res,'wb').write(mon)
    print(res,name,len(mon))
