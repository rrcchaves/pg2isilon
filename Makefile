CXX := g++
SRCDIR := src
BUILDDIR := build
LIBDIR := lib
TARGETDIR := bin
TARGET := MigradorPgIsilon

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CXXFLAGS := -std=c++11 -Wall
LIB :=  -lpq -lpqxx -L $(LIBDIR) -lrestclient-cpp -lcurl -lpthread
INC := -I include

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@mkdir -p ${TARGETDIR}
	@echo " $(CXX) $^ -o $(TARGETDIR)/$(TARGET) $(LIB)"; $(CXX) $^ -o $(TARGETDIR)/$(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<"; $(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning..."
	@echo " $(RM) -r $(BUILDDIR) $(TARGETDIR)/$(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGETDIR)/$(TARGET)

.PHONY: all clean
