INCLUDES = -I$(top_srcdir)/lib -I$(top_builddir)/lib

if BUILD_WITH_MSVC
noinst_LIBRARIES = nodekits.lst
else
noinst_LTLIBRARIES = libnodekits.la
endif

EXTRA_DIST = \
        bitmapfont.cpp

RegularSources = \
	LegendKit.cpp LegendKit.h \
	SoFEMKit.cpp SoFEMKit.h \
	SmTooltipKit.cpp SmTooltipKit.h \
	SmWellLogKit.cpp SmWellLogKit.h \
	SmCameraControlKit.cpp SmCameraControlKit.h \
	GeoMarkerKit.cpp SmGeoMarkerKit.h \
	NormalsKit.cpp SmNormalsKit.h \
	SmAxisDisplayKit.cpp SmAxisDisplayKit.h \
	SmAxisKit.cpp SmAxisKit.h \
	SmAnnotationWall.cpp SmAnnotationWall.h \
	SmAnnotationAxis.cpp SmAnnotationAxis.h \
	SmPieChart.cpp SmPieChart.h \
	PopupMenuKit.cpp SmPopupMenuKit.h \
        SmDynamicObjectKit.cpp SmDynamicObjectKit.h \
        SmTrackPointKit.cpp SmTrackPointKit.h

libnodekits_la_SOURCES = $(RegularSources)

nodekits_lst_SOURCES = $(RegularSources)

libnodekitsincdir = $(includedir)/SmallChange/nodekits
libnodekitsinc_HEADERS = \
	DynamicBaseKit.h \
	DynamicNodeKit.h \
	LegendKit.h \
	SoFEMKit.h \
	SmTooltipKit.h \
	SmWellLogKit.h \
	SmCameraControlKit.h \
	SmGeoMarkerKit.h \
	SmNormalsKit.h \
	SmAxisDisplayKit.h \
	SmAxisKit.h \
	SmAnnotationWall.h \
	SmAnnotationAxis.h \
	SmPieChart.h \
	SmPopupMenuKit.h \
        SmDynamicObjectKit.h \
        SmTrackPointKit.h

nodekits.lst: $(nodekits_lst_OBJECTS)
	@echo "Linking $@..."; \
	rm -f $@; \
	for i in $(nodekits_lst_OBJECTS); do echo $$i >>$@; done

install-libnodekitsincHEADERS: $(libnodekitsinc_HEADERS)
	@$(NORMAL_INSTALL)
	@if test x"$(libnodekitsinc_HEADERS)" = x""; then :; else \
	  echo " $(mkinstalldirs) $(DESTDIR)$(libnodekitsincdir)"; \
	  $(mkinstalldirs) $(DESTDIR)$(libnodekitsincdir); \
	fi
	@list='$(libnodekitsinc_HEADERS)'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  f="`echo $$p | sed -e 's|^.*/||'`"; \
	  src="$$d$$p"; dst="$(DESTDIR)$(libnodekitsincdir)/$$f"; \
	  if cmp -s "$$src" "$$dst"; then :; else \
	    echo " $(INSTALL_HEADER) $$src $$dst"; \
	    $(INSTALL_HEADER) "$$src" "$$dst" || exit 1; \
	  fi \
	done
	@list='$(ObsoletedHeaders)'; for f in $$list; do \
	  file="$(DESTDIR)$(libnodekitsincdir)/$$f"; \
	  if test -x "$$file"; then \
	    echo " rm -f $$file"; \
	    rm -f "$$file"; \
	  fi \
	done; \
	:

