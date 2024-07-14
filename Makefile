# Defina os compiladores e flags
CXX = g++
CXXFLAGS = -I./src/include -std=c++11

# Diretórios das bibliotecas
LDFLAGS = -L./src/lib

# Bibliotecas a serem linkadas
LIBS = -lmingw32 -lSDL2main -lSDL2 -lglew32 -lopengl32 -lglu32 -lws2_32 #-lSDL2_ttf

# Nomes dos executáveis
TARGET_CLIENTE = cliente.exe
TARGET_SERVER = server.exe

# Objetos a serem compilados
OBJS_CLIENTE = Cliente.o main_client.o
OBJS_SERVER = Server.o main_server.o

# Regras para compilar os executáveis
all: $(TARGET_CLIENTE) $(TARGET_SERVER) clean

$(TARGET_CLIENTE): $(OBJS_CLIENTE)
	$(CXX) $(LDFLAGS) -o $(TARGET_CLIENTE) $(OBJS_CLIENTE) $(LIBS)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CXX) $(LDFLAGS) -o $(TARGET_SERVER) $(OBJS_SERVER) $(LIBS)

# Regras para compilar os arquivos .cpp
Cliente.o: Cliente/NetworkClient.cpp Cliente/NetworkClient.h
	$(CXX) $(CXXFLAGS) -c Cliente/NetworkClient.cpp -o Cliente.o

Server.o: Server/Server.cpp Server/Server.h
	$(CXX) $(CXXFLAGS) -c Server/Server.cpp -o Server.o

main_client.o: Cliente/main_client.cpp
	$(CXX) $(CXXFLAGS) -c Cliente/main_client.cpp -o main_client.o

main_server.o: Server/main_server.cpp
	$(CXX) $(CXXFLAGS) -c Server/main_server.cpp -o main_server.o

# Limpeza dos arquivos gerados
clean:
	del /Q *.o
