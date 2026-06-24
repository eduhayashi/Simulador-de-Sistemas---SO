# Nome do compilador
CXX = g++

# Flags de compilação (C++11 habilitado, avisos ligados e flags do GTK3)
CXXFLAGS = -std=c++11 -Wall $(shell pkg-config --cflags gtk+-3.0)

# Bibliotecas e links do GTK3
LIBS = $(shell pkg-config --libs gtk+-3.0)

# Nome do executável final
TARGET = simulador

# Lista de arquivos fonte
SRCS = main.cpp csv_reader.cpp escalonador.cpp memoria.cpp relatorio.cpp interface.cpp

# Lista de arquivos objeto correspondentes
OBJS = $(SRCS:.cpp=.o)

# Regra padrão para compilar o projeto
all: $(TARGET)

# Compila o executável linkando todos os objetos e as libs do GTK3
$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

# Regra genérica para gerar os arquivos objeto (.o) a partir dos arquivos fonte (.cpp)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regra para limpar arquivos temporários de compilação
clean:
	@echo Limpando arquivos objeto e executavel...
	@del /q /f $(OBJS) $(TARGET) $(TARGET).exe 2>nul || rm -f $(OBJS) $(TARGET) $(TARGET).exe

.PHONY: all clean
