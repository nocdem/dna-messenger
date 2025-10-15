# DNA Messenger - Backend Server

Node.js + Express + Socket.IO backend for web messenger.

## Setup

```bash
npm install
```

## Configuration

Create `.env` file:
```env
PORT=8080
DB_HOST=localhost
DB_PORT=5432
DB_NAME=dna_messenger
DB_USER=dna
DB_PASSWORD=dna_password
JWT_SECRET=your-secret-key-here
NODE_ENV=development
```

## Run

```bash
# Development (with hot reload)
npm run dev

# Production
npm start
```

## API Structure

```
src/
├── server.js          # Main entry point
├── websocket.js       # WebSocket handler (Socket.IO)
├── routes/
│   ├── auth.js        # JWT authentication
│   ├── keyserver.js   # Public key management
│   ├── messages.js    # Message routing
│   └── contacts.js    # Contact list
├── db/
│   └── postgres.js    # PostgreSQL connection pool
└── middleware/
    └── auth.js        # JWT verification middleware
```

## Security

- JWT token authentication
- Rate limiting (10 req/sec per IP)
- CORS protection
- Input validation
- Zero-knowledge (never sees plaintext or private keys)
