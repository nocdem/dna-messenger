#!/bin/bash
# Generate Android release keystore for DNA Messenger

set -e

KEYSTORE_DIR="androidApp/keystore"
KEYSTORE_FILE="$KEYSTORE_DIR/dna-messenger-release.keystore"
ALIAS="dna-messenger"

echo "================================"
echo "DNA Messenger Keystore Generator"
echo "================================"
echo ""

# Create keystore directory if it doesn't exist
mkdir -p "$KEYSTORE_DIR"

# Check if keystore already exists
if [ -f "$KEYSTORE_FILE" ]; then
    echo "⚠️  Warning: Keystore already exists at: $KEYSTORE_FILE"
    read -p "Do you want to overwrite it? (yes/no): " confirm
    if [ "$confirm" != "yes" ]; then
        echo "Aborted."
        exit 1
    fi
    rm "$KEYSTORE_FILE"
fi

echo ""
echo "This will create a new keystore with the following settings:"
echo "  - Key algorithm: RSA"
echo "  - Key size: 4096 bits"
echo "  - Validity: 10000 days (~27 years)"
echo "  - Alias: $ALIAS"
echo ""

# Prompt for passwords
read -sp "Enter keystore password: " STORE_PASSWORD
echo ""
read -sp "Confirm keystore password: " STORE_PASSWORD_CONFIRM
echo ""

if [ "$STORE_PASSWORD" != "$STORE_PASSWORD_CONFIRM" ]; then
    echo "❌ Error: Passwords do not match"
    exit 1
fi

read -sp "Enter key password (or press Enter to use same as keystore): " KEY_PASSWORD
echo ""

if [ -z "$KEY_PASSWORD" ]; then
    KEY_PASSWORD="$STORE_PASSWORD"
fi

# Prompt for certificate details
echo ""
echo "Certificate Details:"
read -p "  Your name (CN): " CN
read -p "  Organization (O) [DNA Messenger]: " ORG
ORG=${ORG:-DNA Messenger}
read -p "  Organizational Unit (OU) [Development]: " OU
OU=${OU:-Development}
read -p "  City (L) [Unknown]: " CITY
CITY=${CITY:-Unknown}
read -p "  State (ST) [Unknown]: " STATE
STATE=${STATE:-Unknown}
read -p "  Country Code (C) [US]: " COUNTRY
COUNTRY=${COUNTRY:-US}

# Generate keystore
echo ""
echo "Generating keystore..."
keytool -genkey -v \
    -keystore "$KEYSTORE_FILE" \
    -alias "$ALIAS" \
    -keyalg RSA \
    -keysize 4096 \
    -validity 10000 \
    -storepass "$STORE_PASSWORD" \
    -keypass "$KEY_PASSWORD" \
    -dname "CN=$CN, OU=$OU, O=$ORG, L=$CITY, ST=$STATE, C=$COUNTRY"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Keystore generated successfully!"
    echo ""
    echo "Location: $KEYSTORE_FILE"

    # Create keystore.properties
    PROPERTIES_FILE="keystore.properties"
    echo ""
    read -p "Create keystore.properties file? (yes/no): " create_props

    if [ "$create_props" = "yes" ]; then
        cat > "$PROPERTIES_FILE" <<EOF
# Android Keystore Configuration
# WARNING: Keep this file secure and never commit to version control!

storeFile=$KEYSTORE_FILE
storePassword=$STORE_PASSWORD
keyAlias=$ALIAS
keyPassword=$KEY_PASSWORD
EOF
        echo "✅ Created $PROPERTIES_FILE"
        echo ""
        echo "⚠️  IMPORTANT SECURITY REMINDERS:"
        echo "   1. This file contains sensitive passwords"
        echo "   2. It is already gitignored - DO NOT commit it"
        echo "   3. Back up the keystore file securely"
        echo "   4. Store passwords in a password manager"
        echo "   5. If you lose the keystore, you cannot update the app on Play Store"
    fi

    echo ""
    echo "Next steps:"
    echo "  1. Back up $KEYSTORE_FILE securely"
    echo "  2. Store passwords in a password manager"
    echo "  3. Build release APK: ./gradlew :androidApp:assembleRelease"
    echo "  4. Or build bundle: ./gradlew :androidApp:bundleRelease"
    echo ""

else
    echo "❌ Error: Failed to generate keystore"
    exit 1
fi
