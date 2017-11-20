OBJS = os_specific/service_layers/osraph.o $(addprefix components/, $(addprefix dispatcher/, dsargs.o dsfield.o dsmthdat.o dsutils.o dswload2.o dscontrol.o dsinit.o dsobject.o dswexec.o dswscope.o dsdebug.o dsmethod.o dsopcode.o dswload.o dswstate.o) $(addprefix events/, evevent.o evgpeblk.o evhandler.o evrgnini.o evxfevnt.o evglock.o evgpeinit.o evmisc.o evsci.o evxfgpe.o evgpe.o evgpeutil.o evregion.o evxface.o evxfregn.o) $(addprefix executer/, exconcat.o exdebug.o exmisc.o exoparg2.o exregion.o exstore.o extrace.o exconfig.o exdump.o exmutex.o exoparg3.o exresnte.o exstoren.o exutils.o exconvrt.o exfield.o exnames.o exoparg6.o exresolv.o exstorob.o excreate.o exfldio.o exoparg1.o exprep.o exresop.o exsystem.o) $(addprefix hardware/, hwacpi.o hwgpe.o hwregs.o hwtimer.o hwxface.o hwesleep.o hwpci.o hwsleep.o hwvalid.o hwxfsleep.o) $(addprefix parser/, psargs.o psobject.o psopinfo.o psscope.o psutils.o psxface.o psloop.o psopcode.o psparse.o pstree.o pswalk.o) $(addprefix namespace/, nsaccess.o nsdump.o nsload.o nspredef.o nssearch.o nsxfname.o nsalloc.o nsdumpdv.o nsnames.o nsprepkg.o nsutils.o nsxfobj.o nsarguments.o nseval.o nsobject.o nsrepair.o nswalk.o nsconvert.o nsinit.o nsparse.o nsrepair2.o nsxfeval.o) $(addprefix utilities/, utaddress.o utcopy.o utexcep.o utmath.o utownerid.o uttrack.o utalloc.o utdebug.o utglobal.o utmisc.o utpredef.o utuuid.o utascii.o utdecode.o uthex.o utmutex.o utprint.o utxface.o utbuffer.o utdelete.o utids.o utnonansi.o utresrc.o utxferror.o utcache.o uterror.o utinit.o utobject.o utstate.o utxfinit.o utclib.o uteval.o utlock.o utosi.o utstring.o utxfmutex.o) $(addprefix tables/, tbdata.o tbfind.o tbprint.o tbxface.o tbxfroot.o tbfadt.o tbinstal.o tbutils.o tbxfload.o) $(addprefix resources/, rsaddr.o rscreate.o rsdumpinfo.o rsio.o rslist.o rsmisc.o rsutils.o rscalc.o rsinfo.o rsirq.o rsmemory.o rsserial.o rsxface.o))

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean

include $(RAPH_PROJECT_ROOT)/common_rules.mk

default:

build: ../acpica.o

-include $(DEPS)

../acpica.o: $(OBJS)
	ld -r $^ -o $@

clean:
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
