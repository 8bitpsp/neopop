# Rules
$(PSPLIB)/audio.o:  $(PSPLIB)/audio.c $(PSPLIB)/audio.h
$(PSPLIB)/perf.o:   $(PSPLIB)/perf.c $(PSPLIB)/perf.h
$(PSPLIB)/ctrl.o:   $(PSPLIB)/ctrl.c $(PSPLIB)/ctrl.h
$(PSPLIB)/fileio.o: $(PSPLIB)/fileio.c $(PSPLIB)/fileio.h
$(PSPLIB)/font.o:   $(PSPLIB)/font.c $(PSPLIB)/font.h \
                    $(PSPLIB)/stockfont.h
$(PSPLIB)/image.o:  $(PSPLIB)/image.c $(PSPLIB)/image.h
$(PSPLIB)/kybd.o:   $(PSPLIB)/kybd.c $(PSPLIB)/kybd.h \
                    $(PSPLIB)/ctrl.c $(PSPLIB)/video.c \
                    $(PSPLIB)/image.c $(PSPLIB)/font.c
$(PSPLIB)/menu.o:   $(PSPLIB)/menu.c $(PSPLIB)/menu.h
$(PSPLIB)/init.o:   $(PSPLIB)/init.c $(PSPLIB)/init.h
$(PSPLIB)/psp.o:    $(PSPLIB)/psp.c $(PSPLIB)/psp.h \
                    $(PSPLIB)/fileio.c
$(PSPLIB)/ui.o:     $(PSPLIB)/ui.c $(PSPLIB)/ui.h \
                    $(PSPLIB)/video.c $(PSPLIB)/menu.c \
                    $(PSPLIB)/psp.c $(PSPLIB)/fileio.c \
                    $(PSPLIB)/ctrl.c $(PSPLIB)/font.c
$(PSPLIB)/video.o:  $(PSPLIB)/video.c $(PSPLIB)/video.h \
                    $(PSPLIB)/font.c $(PSPLIB)/image.c
$(PSPLIB)/util.o:   $(PSPLIB)/util.c $(PSPLIB)/util.h \
                    $(PSPLIB)/video.c $(PSPLIB)/fileio.c \
                    $(PSPLIB)/image.c
$(PSPLIB)/stockfont.h: $(PSPLIB)/genfont $(PSPLIB)/stockfont.fd
	$< < $(word 2,$^) > $@
$(PSPLIB)/genfont: $(PSPLIB)/genfont.c
	cc $< -o $@
