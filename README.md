# MiniHTTPd

Servidor HTTP/1.1 escrito en C para Linux. Sirve archivos estaticos
desde el directorio `www/` usando `epoll` para manejar multiples
clientes al mismo tiempo.

## Requisitos

- Linux con gcc y make instalados.

## Compilar y correr

```bash
make
./minihttpd
```

El servidor queda escuchando en `http://localhost:8080`.
Para detenerlo: `Ctrl+C`.

## Flujo
Cliente envia request
↓
server.c → recibe la conexion con epoll y lee el buffer
↓
http.c  → parsea metodo, URI y version
↓
files.c → resuelve la ruta y verifica que este dentro de www/
↓
mime.c  → detecta el tipo de archivo segun su extension
↓
http.c  → envia headers + archivo con sendfile()

## Probar con curl

```bash
# Pagina principal
curl http://localhost:8080/

# Archivo CSS
curl http://localhost:8080/style.css

# Imagen (guardar en archivo porque es binario)
curl http://localhost:8080/image.png --output imagen.png

# Request malformado (400)
echo -e "BASURA\r\n\r\n" | nc localhost 8080

# Intento de directory traversal (403)
curl --path-as-is "http://localhost:8080/../../../../etc/passwd"

# Archivo inexistente (404)
curl -v http://localhost:8080/noexiste.html

# Metodo no permitido (405)
curl -v -X POST http://localhost:8080/
```

## Errores

| Codigo | Descripcion |
|--------|-------------|
| 400    | Request malformado o URL demasiado larga |
| 403    | Acceso denegado fuera del directorio www/ |
| 404    | Archivo no encontrado |
| 405    | Metodo no permitido, solo acepta GET |
| 500    | Error interno del servidor |

## Autor

Francisco Morales
