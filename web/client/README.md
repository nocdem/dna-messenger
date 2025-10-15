# DNA Messenger - Web Client

React frontend for DNA Messenger.

## Setup

```bash
npm install
```

## Development

```bash
npm start
# Opens http://localhost:3000
```

## Build

```bash
npm run build
# Outputs to build/ directory
```

## Features

- Real-time messaging (WebSocket)
- Post-quantum encryption (WebAssembly)
- Contact list with online status
- Message history
- Encrypted key storage (IndexedDB)
- Responsive design (mobile + desktop)

## Component Structure

```
src/
├── components/
│   ├── LoginView.jsx        # Login/registration
│   ├── ContactList.jsx      # Contact sidebar
│   ├── ChatWindow.jsx       # Message display
│   ├── MessageInput.jsx     # Compose message
│   └── KeyManager.jsx       # Key generation/import
├── hooks/
│   ├── useWebSocket.js      # WebSocket connection
│   ├── useCrypto.js         # WASM crypto wrapper
│   └── useKeyStorage.js     # IndexedDB key management
└── utils/
    ├── crypto.js            # Crypto helpers
    └── storage.js           # IndexedDB utilities
```

## Security

- Content Security Policy (CSP)
- Input sanitization (DOMPurify)
- XSS protection
- Private keys never sent to server
- Keys encrypted in IndexedDB with user passphrase
