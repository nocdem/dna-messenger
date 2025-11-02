#!/bin/bash

# DNA Messenger - PostgreSQL Connection Test Script
# Tests database connectivity before deploying APK

set -e

DB_HOST="${1:-ai.cpunk.io}"
DB_PORT="${2:-5432}"
DB_NAME="${3:-dna_messenger}"
DB_USER="${4:-dna}"
DB_PASS="${5:-dna_password}"

echo "========================================="
echo "DNA Messenger - Database Connection Test"
echo "========================================="
echo ""
echo "Testing connection to:"
echo "  Host: $DB_HOST"
echo "  Port: $DB_PORT"
echo "  Database: $DB_NAME"
echo "  User: $DB_USER"
echo ""

# Test 1: DNS Resolution
echo "[1/5] Testing DNS resolution..."
if host "$DB_HOST" > /dev/null 2>&1; then
    IP=$(host "$DB_HOST" | grep "has address" | awk '{print $4}' | head -1)
    echo "✓ DNS resolved: $DB_HOST → $IP"
else
    echo "✗ DNS resolution failed for $DB_HOST"
    exit 1
fi

# Test 2: Network Connectivity
echo ""
echo "[2/5] Testing network connectivity..."
if timeout 5 bash -c "cat < /dev/null > /dev/tcp/$DB_HOST/$DB_PORT" 2>/dev/null; then
    echo "✓ TCP connection successful to $DB_HOST:$DB_PORT"
else
    echo "✗ Cannot connect to $DB_HOST:$DB_PORT"
    echo "  Make sure the port is open and accessible"
    exit 1
fi

# Test 3: PostgreSQL Connection (if psql available)
echo ""
echo "[3/5] Testing PostgreSQL authentication..."
if command -v psql > /dev/null 2>&1; then
    export PGPASSWORD="$DB_PASS"
    if psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "SELECT version();" > /dev/null 2>&1; then
        echo "✓ PostgreSQL authentication successful"
        psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "SELECT version();" | head -3
    else
        echo "✗ PostgreSQL authentication failed"
        echo "  Check username/password: $DB_USER / ****"
        exit 1
    fi
else
    echo "⊘ psql not installed, skipping authentication test"
fi

# Test 4: Check required tables
echo ""
echo "[4/5] Checking database schema..."
if command -v psql > /dev/null 2>&1; then
    export PGPASSWORD="$DB_PASS"
    TABLES=$(psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -t -c "SELECT tablename FROM pg_tables WHERE schemaname='public';" 2>/dev/null || echo "")

    if echo "$TABLES" | grep -q "messages"; then
        echo "✓ Table 'messages' exists"
    else
        echo "✗ Table 'messages' not found"
    fi

    if echo "$TABLES" | grep -q "keyserver"; then
        echo "✓ Table 'keyserver' exists"
    else
        echo "✗ Table 'keyserver' not found"
    fi

    if echo "$TABLES" | grep -q "groups"; then
        echo "✓ Table 'groups' exists"
    else
        echo "✗ Table 'groups' not found"
    fi
else
    echo "⊘ psql not installed, skipping schema check"
fi

echo ""
echo "========================================="
echo "Connection test complete!"
echo "========================================="
