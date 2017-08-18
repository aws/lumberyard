'use strict'
const httpServer = require('http-server')

let cache = 3600
if (process.env.NODE_ENV === 'production') {
  console.log('running in production mode(with caching)-make sure you have "Disable cache (while DevTools is open)" checked in the browser to see the changes while developing')
} else {
  cache = -1
}
const server = httpServer.createServer({
  root: './',
  cache: cache,
  robots: true,
  headers: {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Credentials': 'true'
  }
})

require('chokidar-socket-emitter')({app: server.server})

server.listen(3000)