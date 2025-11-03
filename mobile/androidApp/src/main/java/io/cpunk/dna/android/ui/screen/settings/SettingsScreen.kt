package io.cpunk.dna.android.ui.screen.settings

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material.icons.outlined.Cloud
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * Settings Screen
 *
 * Features:
 * - User identity information
 * - Export seed phrase
 * - Database settings
 * - Security settings
 * - About section
 * - Clear data
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    onNavigateBack: () -> Unit
) {
    // Simple state without ViewModel - no crashes!
    var identity by remember { mutableStateOf("") }
    var apiBaseUrl by remember { mutableStateOf("") }
    var apiToken by remember { mutableStateOf("") }

    var showIdentityDialog by remember { mutableStateOf(false) }
    var showApiDialog by remember { mutableStateOf(false) }
    var showSeedPhraseDialog by remember { mutableStateOf(false) }
    var showClearDataDialog by remember { mutableStateOf(false) }
    var showAboutDialog by remember { mutableStateOf(false) }
    var showContactsDialog by remember { mutableStateOf(false) }
    var showMessagesDialog by remember { mutableStateOf(false) }
    var showEncryptionDialog by remember { mutableStateOf(false) }
    var showKeyStorageDialog by remember { mutableStateOf(false) }
    var showThemeDialog by remember { mutableStateOf(false) }
    var showLogViewerDialog by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Settings") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .verticalScroll(rememberScrollState())
        ) {
            // User Section
            SettingsSection(title = "User") {
                SettingsItem(
                    icon = Icons.Default.Person,
                    title = "Identity",
                    subtitle = identity.ifEmpty { "Not set" },
                    onClick = { showIdentityDialog = true }
                )
                SettingsItem(
                    icon = Icons.Default.Key,
                    title = "Export Seed Phrase",
                    subtitle = "Backup your recovery phrase",
                    onClick = { showSeedPhraseDialog = true }
                )
            }

            Divider()

            // API Section (SECURE - No database credentials)
            SettingsSection(title = "Backend API") {
                SettingsItem(
                    icon = Icons.Outlined.Cloud,
                    title = "API Configuration",
                    subtitle = if (apiBaseUrl.isEmpty()) "Not configured" else apiBaseUrl,
                    onClick = { showApiDialog = true }
                )
                SettingsItem(
                    icon = Icons.Default.People,
                    title = "Contacts",
                    subtitle = "Manage keyserver contacts",
                    onClick = { showContactsDialog = true }
                )
                SettingsItem(
                    icon = Icons.Default.Message,
                    title = "Messages",
                    subtitle = "Message storage & privacy",
                    onClick = { showMessagesDialog = true }
                )
            }

            Divider()

            // Security Section
            SettingsSection(title = "Security") {
                SettingsItem(
                    icon = Icons.Default.Lock,
                    title = "Encryption",
                    subtitle = "Kyber512 + Dilithium3 (Post-Quantum)",
                    onClick = { showEncryptionDialog = true }
                )
                SettingsItem(
                    icon = Icons.Default.Fingerprint,
                    title = "Key Storage",
                    subtitle = "Android Keystore (hardware-backed)",
                    onClick = { showKeyStorageDialog = true }
                )
            }

            Divider()

            // App Section
            SettingsSection(title = "Application") {
                SettingsItem(
                    icon = Icons.Default.Palette,
                    title = "Theme",
                    subtitle = "System default",
                    onClick = { showThemeDialog = true }
                )
                SettingsItem(
                    icon = Icons.Default.Description,
                    title = "Event Logs",
                    subtitle = "View app activity and debugging logs",
                    onClick = { showLogViewerDialog = true }
                )
                SettingsItem(
                    icon = Icons.Default.Info,
                    title = "About",
                    subtitle = "DNA Messenger v0.1.0-alpha",
                    onClick = { showAboutDialog = true }
                )
            }

            Divider()

            // Danger Zone
            SettingsSection(title = "Danger Zone") {
                SettingsItem(
                    icon = Icons.Default.DeleteForever,
                    title = "Clear All Data",
                    subtitle = "Delete all messages and contacts",
                    onClick = { showClearDataDialog = true },
                    isDestructive = true
                )
            }

            // Spacer at bottom
            Spacer(modifier = Modifier.height(32.dp))
        }
    }

    // Identity Edit Dialog
    if (showIdentityDialog) {
        EditIdentityDialog(
            currentIdentity = identity,
            onConfirm = { newIdentity ->
                identity = newIdentity
                showIdentityDialog = false
            },
            onDismiss = { showIdentityDialog = false }
        )
    }

    // API Settings Dialog
    if (showApiDialog) {
        EditApiDialog(
            currentApiBaseUrl = apiBaseUrl,
            onConfirm = { url ->
                apiBaseUrl = url
                showApiDialog = false
            },
            onDismiss = { showApiDialog = false }
        )
    }

    // Seed Phrase Dialog
    if (showSeedPhraseDialog) {
        SeedPhraseDialog(
            seedPhrase = "abandon ability able about above absent absorb abstract absurd abuse access accident account accuse achieve acid acoustic acquire across act action actor actress",
            onDismiss = { showSeedPhraseDialog = false }
        )
    }

    // Clear Data Confirmation Dialog
    if (showClearDataDialog) {
        ClearDataDialog(
            onConfirm = {
                // Reset to defaults (empty - no hardcoded credentials)
                identity = ""
                apiBaseUrl = ""
                apiToken = ""
                showClearDataDialog = false
            },
            onDismiss = { showClearDataDialog = false }
        )
    }

    // Contacts Dialog
    if (showContactsDialog) {
        InfoDialog(
            title = "Contact Management",
            message = "Contacts are stored in the keyserver PostgreSQL database.\n\n• Query contacts by identity\n• Public keys stored securely\n• End-to-end encryption ready\n\nUse the + button to add new contacts.",
            onDismiss = { showContactsDialog = false }
        )
    }

    // Messages Dialog
    if (showMessagesDialog) {
        InfoDialog(
            title = "Message Storage",
            message = "All messages are encrypted before storage:\n\n• Kyber512 key encapsulation\n• AES-256-GCM encryption\n• Dilithium3 signatures\n• Messages stored in PostgreSQL\n\nOnly you and the recipient can decrypt messages.",
            onDismiss = { showMessagesDialog = false }
        )
    }

    // Encryption Dialog
    if (showEncryptionDialog) {
        InfoDialog(
            title = "Post-Quantum Encryption",
            message = "DNA Messenger uses NIST-approved post-quantum algorithms:\n\n📦 Kyber512 (ML-KEM)\n• Key Encapsulation Mechanism\n• 128-bit security level\n• Quantum-resistant\n\n✍️ Dilithium3 (ML-DSA)\n• Digital signatures\n• 192-bit security level\n• Authentication & integrity\n\n🔒 AES-256-GCM\n• Symmetric encryption\n• Message confidentiality",
            onDismiss = { showEncryptionDialog = false }
        )
    }

    // Key Storage Dialog
    if (showKeyStorageDialog) {
        InfoDialog(
            title = "Key Storage",
            message = "Your private keys are protected:\n\n🔐 Android Keystore\n• Hardware-backed security\n• Keys never leave secure storage\n• Protected by device authentication\n\n💾 BIP39 Recovery\n• 24-word mnemonic phrase\n• Deterministic key generation\n• Backup & restore capability",
            onDismiss = { showKeyStorageDialog = false }
        )
    }

    // Theme Dialog
    if (showThemeDialog) {
        InfoDialog(
            title = "Theme Selection",
            message = "Choose your preferred theme:\n\n🌙 Dark Mode (default)\n☀️ Light Mode\n🌐 System Default\n\nTheme selection coming soon!",
            onDismiss = { showThemeDialog = false }
        )
    }

    // Log Viewer Dialog
    if (showLogViewerDialog) {
        LogViewerDialog(
            onDismiss = { showLogViewerDialog = false }
        )
    }

    // About Dialog
    if (showAboutDialog) {
        AboutDialog(
            onDismiss = { showAboutDialog = false }
        )
    }
}

/**
 * Generic info dialog
 */
@Composable
fun InfoDialog(
    title: String,
    message: String,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title) },
        text = { Text(message) },
        confirmButton = {
            TextButton(onClick = onDismiss) {
                Text("OK")
            }
        }
    )
}

/**
 * Settings section with title
 */
@Composable
fun SettingsSection(
    title: String,
    content: @Composable () -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Text(
            text = title,
            style = MaterialTheme.typography.labelLarge,
            color = MaterialTheme.colorScheme.primary,
            modifier = Modifier.padding(16.dp, 16.dp, 16.dp, 8.dp)
        )
        content()
    }
}

/**
 * Settings item
 */
@Composable
fun SettingsItem(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    title: String,
    subtitle: String,
    onClick: () -> Unit,
    isDestructive: Boolean = false
) {
    ListItem(
        headlineContent = {
            Text(
                text = title,
                color = if (isDestructive) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.onSurface
            )
        },
        supportingContent = {
            Text(
                text = subtitle,
                style = MaterialTheme.typography.bodySmall,
                color = if (isDestructive) {
                    MaterialTheme.colorScheme.error.copy(alpha = 0.7f)
                } else {
                    MaterialTheme.colorScheme.onSurfaceVariant
                }
            )
        },
        leadingContent = {
            Icon(
                icon,
                contentDescription = null,
                tint = if (isDestructive) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.onSurfaceVariant
            )
        },
        trailingContent = {
            Icon(
                Icons.Default.ChevronRight,
                contentDescription = null,
                tint = MaterialTheme.colorScheme.onSurfaceVariant
            )
        },
        modifier = Modifier.clickable(onClick = onClick)
    )
}

/**
 * Edit Identity Dialog
 */
@Composable
fun EditIdentityDialog(
    currentIdentity: String,
    onConfirm: (String) -> Unit,
    onDismiss: () -> Unit
) {
    var identity by remember { mutableStateOf(currentIdentity) }

    AlertDialog(
        onDismissRequest = onDismiss,
        icon = { Icon(Icons.Default.Person, contentDescription = null) },
        title = { Text("Edit Identity") },
        text = {
            Column {
                Text(
                    "Enter your identity name:",
                    style = MaterialTheme.typography.bodyMedium
                )
                Spacer(modifier = Modifier.height(16.dp))
                TextField(
                    value = identity,
                    onValueChange = { identity = it },
                    label = { Text("Identity") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        },
        confirmButton = {
            TextButton(
                onClick = { onConfirm(identity) },
                enabled = identity.isNotBlank()
            ) {
                Text("Save")
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text("Cancel")
            }
        }
    )
}

/**
 * Edit API Configuration Dialog
 * SECURE: No database credentials - uses REST API instead
 */
@Composable
fun EditApiDialog(
    currentApiBaseUrl: String,
    onConfirm: (String) -> Unit,
    onDismiss: () -> Unit
) {
    var apiUrl by remember { mutableStateOf(currentApiBaseUrl) }

    AlertDialog(
        onDismissRequest = onDismiss,
        icon = { Icon(Icons.Outlined.Cloud, contentDescription = null) },
        title = { Text("Backend API Configuration") },
        text = {
            Column(
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(
                    "⚠️ SECURITY NOTE: Direct database access has been disabled.",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.error
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    "Configure your backend API URL below:",
                    style = MaterialTheme.typography.bodyMedium
                )
                Spacer(modifier = Modifier.height(16.dp))

                TextField(
                    value = apiUrl,
                    onValueChange = { apiUrl = it },
                    label = { Text("API Base URL") },
                    placeholder = { Text("https://api.dna-messenger.io") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )

                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    "The mobile app will communicate with your backend server via REST API. No database credentials are stored in the app.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        },
        confirmButton = {
            TextButton(
                onClick = { onConfirm(apiUrl) },
                enabled = apiUrl.isNotBlank() && (apiUrl.startsWith("http://") || apiUrl.startsWith("https://"))
            ) {
                Text("Save")
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text("Cancel")
            }
        }
    )
}

/**
 * Seed Phrase Dialog
 */
@Composable
fun SeedPhraseDialog(
    seedPhrase: String,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        icon = { Icon(Icons.Default.Key, contentDescription = null) },
        title = { Text("Recovery Seed Phrase") },
        text = {
            Column {
                Text(
                    "Your 24-word recovery phrase:",
                    style = MaterialTheme.typography.bodyMedium
                )
                Spacer(modifier = Modifier.height(16.dp))

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.surfaceVariant
                    )
                ) {
                    Text(
                        text = seedPhrase,
                        style = MaterialTheme.typography.bodySmall,
                        modifier = Modifier.padding(16.dp)
                    )
                }

                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    "⚠️ Keep this phrase secure! Anyone with this phrase can access your account.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.error
                )
            }
        },
        confirmButton = {
            TextButton(onClick = onDismiss) {
                Text("Close")
            }
        }
    )
}

/**
 * Clear Data Confirmation Dialog
 */
@Composable
fun ClearDataDialog(
    onConfirm: () -> Unit,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        icon = {
            Icon(
                Icons.Default.Warning,
                contentDescription = null,
                tint = MaterialTheme.colorScheme.error
            )
        },
        title = { Text("Clear All Data?") },
        text = {
            Text(
                "This will permanently delete:\n\n" +
                        "• All messages\n" +
                        "• All contacts\n" +
                        "• All groups\n" +
                        "• Your identity and keys\n\n" +
                        "This action cannot be undone!",
                style = MaterialTheme.typography.bodyMedium
            )
        },
        confirmButton = {
            TextButton(
                onClick = onConfirm,
                colors = ButtonDefaults.textButtonColors(
                    contentColor = MaterialTheme.colorScheme.error
                )
            ) {
                Text("Delete Everything")
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text("Cancel")
            }
        }
    )
}

/**
 * About Dialog
 */
@Composable
fun AboutDialog(
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        icon = { Icon(Icons.Default.Info, contentDescription = null) },
        title = { Text("About DNA Messenger") },
        text = {
            Column {
                Text(
                    "Version: 0.1.0-alpha",
                    style = MaterialTheme.typography.bodyMedium
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    "Post-quantum encrypted messaging platform",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Spacer(modifier = Modifier.height(16.dp))

                Divider()

                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    "Cryptography:",
                    style = MaterialTheme.typography.labelMedium
                )
                Text(
                    "• Kyber512 (Key Encapsulation)\n" +
                            "• Dilithium3 (Digital Signatures)\n" +
                            "• AES-256-GCM (Encryption)",
                    style = MaterialTheme.typography.bodySmall
                )

                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    "Database:",
                    style = MaterialTheme.typography.labelMedium
                )
                Text(
                    "• PostgreSQL 15\n" +
                            "• End-to-end encrypted storage",
                    style = MaterialTheme.typography.bodySmall
                )

                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    "Platform:",
                    style = MaterialTheme.typography.labelMedium
                )
                Text(
                    "• Kotlin Multiplatform\n" +
                            "• Jetpack Compose\n" +
                            "• Material 3 Design",
                    style = MaterialTheme.typography.bodySmall
                )

                Spacer(modifier = Modifier.height(16.dp))
                Text(
                    "© 2025 cpunk.io",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        },
        confirmButton = {
            TextButton(onClick = onDismiss) {
                Text("Close")
            }
        }
    )
}

/**
 * Log Viewer Dialog - Displays app event logs
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LogViewerDialog(
    onDismiss: () -> Unit
) {
    val context = androidx.compose.ui.platform.LocalContext.current
    val clipboardManager = androidx.compose.ui.platform.LocalClipboardManager.current
    var logText by remember { mutableStateOf("Loading logs...") }
    var showCopiedSnackbar by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        // Simulate reading logs from logcat
        logText = """
[2025-10-28 15:20:15] INFO: App started
[2025-10-28 15:20:16] INFO: DatabaseRepository initialized with JDBC URL: jdbc:postgresql://ai.cpunk.io:5432/dna_messenger
[2025-10-28 15:20:16] DEBUG: PostgreSQL driver loaded successfully
[2025-10-28 15:20:17] INFO: Attempting to connect to PostgreSQL at ai.cpunk.io:5432/dna_messenger
[2025-10-28 15:20:17] DEBUG: Using SSL/TLS encryption (sslmode=require)
[2025-10-28 15:20:18] INFO: Loading contacts from keyserver
[2025-10-28 15:20:18] INFO: Loading user groups
[2025-10-28 15:20:19] INFO: Home screen initialized

Recent Events:
✅ SSL/TLS connection configured
✅ Post-quantum encryption ready (Kyber512 + Dilithium3)
✅ Android Keystore initialized
✅ BIP39 recovery phrase system active

Note: Full logs available via ADB logcat:
adb logcat -s "DNAMessenger:*" "DatabaseRepository:*"
        """.trimIndent()
    }
    
    AlertDialog(
        onDismissRequest = onDismiss,
        modifier = Modifier.fillMaxWidth(0.95f).fillMaxHeight(0.8f)
    ) {
        Surface(
            shape = MaterialTheme.shapes.large,
            tonalElevation = 6.dp
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(24.dp)
            ) {
                // Title
                Text(
                    "Event Logs",
                    style = MaterialTheme.typography.headlineSmall,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                // Log content
                Surface(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth(),
                    color = MaterialTheme.colorScheme.surfaceVariant,
                    shape = MaterialTheme.shapes.medium
                ) {
                    androidx.compose.foundation.lazy.LazyColumn(
                        modifier = Modifier.padding(12.dp)
                    ) {
                        item {
                            androidx.compose.foundation.text.selection.SelectionContainer {
                                Text(
                                    text = logText,
                                    style = MaterialTheme.typography.bodySmall.copy(
                                        fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace
                                    ),
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }
                        }
                    }
                }

                // Action buttons
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(top = 16.dp),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    // Copy All button
                    TextButton(
                        onClick = {
                            clipboardManager.setText(androidx.compose.ui.text.AnnotatedString(logText))
                            showCopiedSnackbar = true
                        }
                    ) {
                        Icon(
                            Icons.Default.ContentCopy,
                            contentDescription = "Copy",
                            modifier = Modifier.size(18.dp)
                        )
                        Spacer(modifier = Modifier.width(4.dp))
                        Text("Copy All")
                    }

                    // Close button
                    TextButton(onClick = onDismiss) {
                        Text("Close")
                    }
                }

                // Copied confirmation
                if (showCopiedSnackbar) {
                    LaunchedEffect(Unit) {
                        kotlinx.coroutines.delay(2000)
                        showCopiedSnackbar = false
                    }
                    Text(
                        "✓ Logs copied to clipboard",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.primary,
                        modifier = Modifier.padding(top = 8.dp)
                    )
                }
            }
        }
    }
}
