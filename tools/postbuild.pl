#!/usr/bin/perl
# Post-build step for "GeoWorks 1.x Test" (pcgeos-1-sdk-v2, ensemble12 backend),
# built like the PDF-Betrachter and GeoMaler:
#  * menu "Datei": own group (Bericht speichern, In Zwischenablage kopieren,
#    Neu messen, Info) + PrintControl of the spool library (Drucken...);
#    Motif merges both into one "Datei" menu and adds Verlassen
#  * menu "Seite" with the ten pages, menu "Hilfe"
#  * view grows with the window
#  * application icon (moniker list + six bitmap resources)
# usage: postbuild.pl in.geo out.geo icon_dir width height
use strict; use warnings;
use lib (($ENV{SDK} || "../pcgeos-1-sdk-v2") . "/tools/lib");
use PCGEOS::Geos1::Format qw(from_bytes);
use PCGEOS::Geos1::Writer qw(build_geode);
use PCGEOS::Ensemble12::Objects qw(object_block text_moniker SAME_BLOCK);
my ($in,$out,$dir,$w,$h)=@ARGV; die "usage\n" unless $h;
sub slurp {open my $f,'<:raw',$_[0] or die "$_[0]: $!"; local $/; <$f>}
sub set16 {my ($r,$at,$v)=@_; substr($$r,$at,2)=pack('v',$v)}
sub base {my ($ord,%o)=@_; pack('v14',0x3000,$ord,0,8,0,0,@{$o{sibling}//[0,0]},@{$o{child}//[0,0]},$o{moniker}//0,$o{accel}//0,$o{hints}//0,$o{states}//0xc000)}
sub moniker {my ($text,$mn)=@_; pack('vvCC',1,0,0,$mn//0xff).$text."\0"}
my $g=from_bytes(slurp($in),$in);
my @res=map {{data=>$_->{data},flags=>$_->{flags},relocations=>[map {[$_->{info},$_->{extra},$_->{offset}]} @{$_->{relocations}}]}} @{$g->{resources}};
die "expected the 5 ensemble12 resources\n" unless @res==5;
# variables start at DGROUP 0xCC: the method table (0x52 + 6 per method) must end there
my $nm=ord(substr($res[1]{data},0x46,1));
die "too many methods ($nm): method table would overlap the variables at 0xCC\n" if 0x52+6*$nm>0xCC;

# ---- own Vis class (subclass of VisContentClass = UI export A3h), built like
# the class of KIDSDRAW.GEO: superclass, 4, method count, instance size 56h,
# 0,0,0, flags 0300h, messages, handlers (offset, code resource 2).
# The table "G1VISCLS" in the code gives messages and handler offsets.
use constant VIS_CLASS=>0x100;
my $code=$res[2]{data}; my $vt=index($code,'G1VISCLS'); die "G1VISCLS table missing\n" if $vt<0;
my $vn=unpack('v',substr($code,$vt+8,2)); my @vm=unpack("v$vn",substr($code,$vt+10,2*$vn)); my @vh=unpack("v$vn",substr($code,$vt+10+2*$vn,2*$vn));
if ($ENV{VIS_ONLY}) {   # test builds: only these messages in the class
  my %keep=map {hex($_)=>1} split /,/,$ENV{VIS_ONLY}; my (@m,@h);
  for my $i (0..$#vm) { if ($keep{$vm[$i]}) { push @m,$vm[$i]; push @h,$vh[$i] } }
  @vm=@m; @vh=@h; $vn=@vm; print "Vis class: ",join(' ',map {sprintf '%04x',$_} @vm),"\n"; }
{ my $dg=$res[1]{data}; die "DGROUP must be 256 bytes\n" unless length($dg)==0x100;
  my $cls=pack('v9',0xa3,0xa3,4,$vn,0x56,0,0,0,0x0300).pack("v$vn",@vm);
  my @rel=([0x11,0,VIS_CLASS],[0x12,0,VIS_CLASS+2]);
  for my $i (0..$vn-1) { push @rel,[0x22,0,VIS_CLASS+length($cls)+2]; $cls.=pack('vv',$vh[$i],2); }
  $cls.="\0" x ((16-length($cls)%16)%16);
  $res[1]{data}=$dg.$cls; push @{$res[1]{relocations}},@rel;
  # DGROUP+14h = bottom of the process stack = end of the initialized data
  # (KIDSDRAW: 1CAh, PUZZLE: 292h). The SDK writes 100h; left so, the stack
  # would run over the class record behind 100h -> "bad handle passed to system"
  substr($res[1]{data},0x14,2)=pack('v',length($res[1]{data})); }
# the object: instance of KIDSDRAW's drawing content (92 bytes), with our class
# (6001h,0 = export 0 of this geode), bounds and the view (28h in resource 3)
my $inst=pack('H*','01600000060000000000f3018f01d878f000000000000000000000000000000000020000000000000046000d1000000000000000000000000000aa00a900000000000000ac00ab00000000000000b200b100000000000000b400b300');
{ my $at=index($inst,pack('vv',0x46,0x100d)); die "view link not found\n" if $at<0; substr($inst,$at,4)=pack('vv',0x28,0x1003);
  substr($inst,10,4)=pack('vv',$w-1,$h-1);
  # Vis part (from byte 6) +2Dh: 80h = same width as the view, 40h = same
  # height (UI.GEO, VisContent size handler) -> the object follows the view
  # The view (D010h) does not grow beyond the size the object reports. So
  # the object is larger than any screen: 4000 x 3000 (tested in GeoWorks
  # with 2000 x 1500: the view fills the window, the mouse grab works).
  # The flags "same size as view" (Vis+2Dh, 80h/40h) do not help.
  substr($inst,10,4)=pack('vv',3999,2999); }
my $title=$g->{long_name}; my $token=pack('H*',substr($g->{token_hex},0,8));

# ---- interface block as the SDK builds it (checked) ----
my ($primary_h,$title_h,$view_h,$app_h)=(0x24,0x26,0x28,0x24);
my $primary=base(88,sibling=>[$app_h|1,0x1004],child=>[$view_h,SAME_BLOCK],moniker=>$title_h)."\0" x 5;
set16(\$primary,0x1c,0x22); set16(\$primary,0x1e,0x80);
my $mkview=sub {my $sib=shift; my $v=base(75,sibling=>$sib); $v.="\0" x (71-length($v));
  my %f=(0x20=>1,0x24=>1,0x2a=>15,0x2e=>0x8000,0x32=>$w,0x34=>$h,0x36=>$w,0x38=>$h,0x3a=>10,0x3c=>10,0x3e=>20,0x40=>15,0x44=>0x1000);
  set16(\$v,$_,$f{$_}) for keys %f; substr($v,0x46,1)=pack('C',1); $v};
my $check=object_block(3,{$primary_h=>[$primary,3],$title_h=>[text_moniker($title),2],$view_h=>[$mkview->([$primary_h|1,SAME_BLOCK]),7]},6);
die "interface block differs from the SDK build - layout assumption broken\n" unless $check eq $res[3]{data};

# ---- chunk handles (must match PC_CHUNK in gwtest.nasm) ----
# GEOS 1 character set: 0x86 = U-umlaut, 0x9a = o-umlaut, 0x8a = a-umlaut
my @fitems=(['Bericht speichern',0x0c00,0],['In Zwischenablage kopieren',0x0c01,3],
            ['Neu messen',0x0c05,0],['Info',0x0c03,0]);
my @pages=(["\x86bersicht",0],['System',1],['Speicher',2],['Bildschirm',0],
           ['GEOS',0],['Leistung',0],['Grafik',0],['Eingabe',0],['Schriften',0],['Klang',0]);
my $next=0x2a; my $alloc=sub {my $hh=$next; $next+=2; $hh};
my ($fm,$fm_m,$fm_hints)=($alloc->(),$alloc->(),$alloc->());
my @fh=map {[$alloc->(),$alloc->()]} @fitems;
my ($pc_h,$pc_hints,$pc_x)=($alloc->(),$alloc->(),$alloc->());
die "PrintControl chunk moved: update PC_CHUNK\n" unless $pc_h==0x40;
my ($sm,$sm_m,$sm_hints)=($alloc->(),$alloc->(),$alloc->());
my @ph=map {[$alloc->(),$alloc->()]} @pages;
my ($hm,$hm_m,$hm_hints,$ht,$ht_m)=map {$alloc->()} 1..5;
my $vh=$alloc->();

my %chunks=($primary_h=>[$primary,3],$title_h=>[text_moniker($title),2]);
$chunks{$view_h}=[$mkview->([$fm,SAME_BLOCK]),7];
# Datei: hint 0x4047 = group belongs in the file menu; 0x4005/6/7 menu form
$chunks{$fm}=[base(80,sibling=>[$pc_h,SAME_BLOCK],child=>[$fh[0][0],SAME_BLOCK],hints=>$fm_hints).pack('C',2),3];
$chunks{$fm_m}=["\0",2]; $chunks{$fm_hints}=[pack('v8',0x4005,4,0x4047,4,0x4006,4,0x4007,4),2];
for my $i (0..$#fitems) {
  my ($label,$msg,$mn)=@{$fitems[$i]}; my ($th,$mh)=@{$fh[$i]};
  my $sib=$i<$#fitems ? [$fh[$i+1][0],SAME_BLOCK] : [$fm|1,SAME_BLOCK];
  $chunks{$th}=[base(68,sibling=>$sib,moniker=>$mh,states=>0xc004).pack('vvv',0,0x1000,$msg),7];   # output = own process
  $chunks{$mh}=[moniker($label,$mn),2];
}
# PrintControl like the one of the PDF-Betrachter; class from import 1 (spool),
# attributes 0x2c ("Drucken..." with dialog); it sends 0x0c02 with the GString to us
my $pc=pack('v5',0x3001,1,0,0x0a,0x1f).pack('v10',0,0,$sm,SAME_BLOCK,0,0,0,0,$pc_hints,0xc000).pack('C',2)
      .pack("v20",0,0,0x2c,1,1,0,0x7fff,0,0x240,0x2d0,0,0,0,0,0x0c02,1,0,0x1000,0,0x1000);
die "PrintControl size\n" unless length($pc)==71;
$chunks{$pc_h}=[$pc,3]; $chunks{$pc_hints}=[pack('v8',0x4005,4,0x4047,4,0x4006,4,0x4007,4),2];
$chunks{$pc_x}=["\0",2];
# Seite
$chunks{$sm}=[base(80,sibling=>[$hm,SAME_BLOCK],child=>[$ph[0][0],SAME_BLOCK],moniker=>$sm_m,hints=>$sm_hints).pack('C',2),3];
$chunks{$sm_m}=[moniker('Seite',0),2]; $chunks{$sm_hints}=[pack('vv',0x4005,4),2];
for my $i (0..$#pages) {
  my ($label,$mn)=@{$pages[$i]}; my ($th,$mh)=@{$ph[$i]};
  my $sib=$i<$#pages ? [$ph[$i+1][0],SAME_BLOCK] : [$sm|1,SAME_BLOCK];
  $chunks{$th}=[base(68,sibling=>$sib,moniker=>$mh,states=>0xc004).pack('vvv',0,0x1000,0x0c10+$i),7];
  $chunks{$mh}=[moniker((($i+1)%10).' '.$label,0),2];
}
# Hilfe
$chunks{$hm}=[base(80,sibling=>[$primary_h|1,SAME_BLOCK],child=>[$ht,SAME_BLOCK],moniker=>$hm_m,hints=>$hm_hints).pack('C',2),3];
$chunks{$hm_m}=[moniker('Hilfe',0),2]; $chunks{$hm_hints}=[pack('vv',0x4005,4),2];
$chunks{$ht}=[base(68,sibling=>[$hm|1,SAME_BLOCK],moniker=>$ht_m,states=>0xc004).pack('vvv',0,0x1000,0x0c04),7];
$chunks{$ht_m}=[moniker('Hilfe zu GeoWorks 1.x Test',0),2];
# view grows with the window (hints 0x4015/0x4014)
{ my $v=$chunks{$view_h}[0]; unless ($ENV{NOGROW}) { set16(\$v,0x36,10000); set16(\$v,0x38,10000); set16(\$v,0x18,$vh); }
  set16(\$v,0x42,0x22); set16(\$v,0x44,0x100b);   # output: our Vis object (chunk 22h, resource 11)
  # view attributes as the content views of PUZZLE (D010h): needed for the
  # mouse grab (with 8000h no DRAG/END_SELECT arrive)
  set16(\$v,0x2e,hex($ENV{VIEWATTR}||'d010'));
  $chunks{$view_h}[0]=$v; }
$chunks{$vh}=[pack('v4',0x4015,4,0x4014,4),2];
my $count=($vh+2-0x20)/2; $count++ if $count%2;
$res[3]{data}=object_block(3,\%chunks,$count);

# ---- application block with icon moniker list (resource 4) ----
my $app=pack('v14',0x3000,71,0,8,0,0,0,0,0x24,0x1003,0x2a,0,0,0x4000);
$app.="\0" x (198-length($app));
set16(\$app,28,0x28); substr($app,30,4)=$token; substr($app,197,1)=pack('C',0x80);
my @types=([5,0x20c6],[6,0x20c1],[7,0x10c6],[8,0x10c1],[9,0x20e1],[10,0x10e1]);
my $list=pack('vvv',0x1081,0x26,0x4000); $list.=pack('vvv',$_->[1],0x22,0x1000|$_->[0]) for @types;
$res[4]{data}=object_block(4,{0x24=>[$app,3],0x26=>[text_moniker($title),2],0x28=>[pack('v4',0x24,0x1003,0,0),2],0x2a=>[$list,2]},6);
for my $t (@types) {
  my ($n)=@$t; my $b=object_block($n,{0x22=>[slurp("$dir/mon_$n.bin"),2]},2);
  substr($b,4,2)=pack('v',0xc000); push @res,{data=>$b,flags=>0x1c7a,relocations=>[[0x23,0,0]]};
}
# resource 11: object block with the Vis object, run by our process
# (flags 041Ah as KIDSDRAW's block)
die "Vis object must be resource 11\n" unless @res==11;
{ my $ob=object_block(11,{0x22=>[$inst,3]},2);
  my $fp=unpack('v',substr($ob,0x20,2)); substr($ob,$fp+1,1)=pack('C',3);   # object_block marks 22h as data
  push @res,{data=>$ob,flags=>0x41a,relocations=>[[0x23,0,0]]}; }
my $new=build_geode(name=>$g->{name},extension=>$g->{extension},long_name=>$title,token=>$token,
  attributes=>$g->{attributes},file_type=>1,release=>$g->{release},protocol=>$g->{protocol},
  kernel_protocol=>$g->{kernel_protocol},udata_size=>4096,   # stack: Vis messages nest deeper (SDK: 2048)
  resources=>\@res,
  imports=>[(map {{name=>$_->{name},attributes=>$_->{attributes},protocol=>$_->{protocol}}} @{$g->{imports}}),
            {name=>'spool',attributes=>16384,protocol=>[613,0]}],
  process_class=>[1,0x40],application_object=>[4,0x24],exports=>[[1,VIS_CLASS]]);
open my $f,'>:raw',$out or die; print $f $new->{raw}; close $f;
printf "%s: %d bytes, %d resources, %d methods\n",$out,length($new->{raw}),scalar @res,$nm;
