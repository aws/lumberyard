'use strict'
const http = require('http')
const fs = require('fs')
const url = require('url')
const mime = require('mime')
const path = require('path')

const root = './'
let cache = 3600
if (process.env.NODE_ENV === 'production') {
    console.log('running in production mode(with caching)-make sure you have "Disable cache (while DevTools is open)" checked in the browser to see the changes while developing')
} else {
    cache = -1
}

const server = http.createServer(function (req, res) {
    let file = path.join(root, req.url === '/' ? '/index.html' : url.parse(req.url).pathname)
    let contentType = mime.lookup(file, 'application/octet-stream')
    let charSet = mime.charsets.lookup(contentType, 'utf-8')
    contentType += `; charset=${charSet}`

    res.setHeader('Content-Type', contentType)
    res.setHeader('access-control-allow-credentials', 'true')
    res.setHeader('access-control-allow-origin', '*')
    res.setHeader('access-control-allow-headers', 'Origin, X-Requested-With, Content-Type, Accept')
    res.setHeader('cache-control', cache == -1 ? 'no-cache, no-store, must-revalidate' : 'max-age=${cache}')

    if (req.method === 'GET') {
        fs.readFile(file, function (err, data) {
            if (err) {
                res.statusCode = 404
                res.end('Failed to read ${file} : ${err.code}\n')
            }
            else {
                res.end(data)
            }
        })

        return
    }
      
    res.end()
})

const io = require('socket.io')(server)
io.on('connection', (socket) => {
    socket.on('client', (name) => {
        console.log('connected client: ' + name)
    })
})

console.log("Server has been started on localhost:3000")
server.listen(3000)