-- DNA Messenger Logging API - PostgreSQL Schema
-- Date: 2025-10-28

-- Event types enumeration
CREATE TYPE event_type AS ENUM (
    'message_sent',
    'message_received',
    'message_failed',
    'connection_success',
    'connection_failed',
    'auth_success',
    'auth_failed',
    'key_generated',
    'key_exported',
    'group_created',
    'group_joined',
    'group_left',
    'contact_added',
    'contact_removed',
    'app_started',
    'app_stopped',
    'error',
    'warning',
    'info',
    'debug'
);

-- Severity levels
CREATE TYPE severity_level AS ENUM (
    'debug',
    'info',
    'warning',
    'error',
    'critical'
);

-- Main logging events table
CREATE TABLE IF NOT EXISTS logging_events (
    id BIGSERIAL PRIMARY KEY,

    -- Event identification
    event_type event_type NOT NULL,
    severity severity_level NOT NULL DEFAULT 'info',

    -- User/identity (nullable for system events)
    identity VARCHAR(32),

    -- Event details
    message TEXT NOT NULL,
    details JSONB,  -- Flexible JSON for additional data

    -- Metadata
    client_ip INET,
    user_agent TEXT,
    platform VARCHAR(50),  -- 'android', 'ios', 'desktop', 'keyserver'
    app_version VARCHAR(50),

    -- Timestamps
    created_at TIMESTAMP DEFAULT NOW(),
    client_timestamp BIGINT,  -- Unix timestamp from client

    -- Optional reference to other tables
    message_id BIGINT,
    group_id INTEGER,

    -- Constraints
    CONSTRAINT valid_timestamp CHECK (client_timestamp IS NULL OR client_timestamp > 0)
);

-- Message-specific logging table (detailed message logs)
CREATE TABLE IF NOT EXISTS logging_messages (
    id BIGSERIAL PRIMARY KEY,

    -- Message identification
    message_id BIGINT UNIQUE,  -- Reference to messages table

    -- Participants
    sender VARCHAR(32) NOT NULL,
    recipient VARCHAR(32) NOT NULL,
    group_id INTEGER,

    -- Status
    status VARCHAR(20) NOT NULL,  -- 'sent', 'delivered', 'read', 'failed'

    -- Size metrics
    plaintext_size INTEGER,
    ciphertext_size INTEGER,

    -- Timing
    encrypted_at TIMESTAMP,
    sent_at TIMESTAMP,
    delivered_at TIMESTAMP,
    read_at TIMESTAMP,

    -- Error info (if failed)
    error_code VARCHAR(50),
    error_message TEXT,

    -- Metadata
    client_ip INET,
    platform VARCHAR(50),

    created_at TIMESTAMP DEFAULT NOW()
);

-- Connection/network logging table
CREATE TABLE IF NOT EXISTS logging_connections (
    id BIGSERIAL PRIMARY KEY,

    -- Connection details
    identity VARCHAR(32),
    connection_type VARCHAR(50),  -- 'database', 'keyserver', 'rpc', 'peer'

    -- Target
    host VARCHAR(255) NOT NULL,
    port INTEGER NOT NULL,

    -- Status
    success BOOLEAN NOT NULL,
    response_time_ms INTEGER,

    -- Error info (if failed)
    error_code VARCHAR(50),
    error_message TEXT,

    -- Metadata
    client_ip INET,
    platform VARCHAR(50),
    app_version VARCHAR(50),

    created_at TIMESTAMP DEFAULT NOW()
);

-- Statistics aggregation table (for performance)
CREATE TABLE IF NOT EXISTS logging_stats (
    id SERIAL PRIMARY KEY,

    -- Period
    period_start TIMESTAMP NOT NULL,
    period_end TIMESTAMP NOT NULL,

    -- Statistics
    total_events BIGINT DEFAULT 0,
    total_messages BIGINT DEFAULT 0,
    total_connections BIGINT DEFAULT 0,

    -- Breakdown by type
    messages_sent BIGINT DEFAULT 0,
    messages_delivered BIGINT DEFAULT 0,
    messages_failed BIGINT DEFAULT 0,

    connections_success BIGINT DEFAULT 0,
    connections_failed BIGINT DEFAULT 0,

    errors_count BIGINT DEFAULT 0,
    warnings_count BIGINT DEFAULT 0,

    -- Metadata
    computed_at TIMESTAMP DEFAULT NOW(),

    UNIQUE(period_start, period_end)
);

-- Indexes for performance
CREATE INDEX idx_events_type ON logging_events(event_type);
CREATE INDEX idx_events_identity ON logging_events(identity);
CREATE INDEX idx_events_created_at ON logging_events(created_at DESC);
CREATE INDEX idx_events_severity ON logging_events(severity);
CREATE INDEX idx_events_platform ON logging_events(platform);

CREATE INDEX idx_messages_sender ON logging_messages(sender);
CREATE INDEX idx_messages_recipient ON logging_messages(recipient);
CREATE INDEX idx_messages_status ON logging_messages(status);
CREATE INDEX idx_messages_sent_at ON logging_messages(sent_at DESC);

CREATE INDEX idx_connections_identity ON logging_connections(identity);
CREATE INDEX idx_connections_success ON logging_connections(success);
CREATE INDEX idx_connections_created_at ON logging_connections(created_at DESC);
CREATE INDEX idx_connections_host ON logging_connections(host);

-- JSONB index for flexible querying
CREATE INDEX idx_events_details ON logging_events USING GIN (details);

-- Partitioning helper (optional - for high-volume deployments)
-- CREATE TABLE logging_events_2025_10 PARTITION OF logging_events
--     FOR VALUES FROM ('2025-10-01') TO ('2025-11-01');

-- Auto-cleanup old logs (retention policy)
CREATE OR REPLACE FUNCTION cleanup_old_logs()
RETURNS void AS $$
BEGIN
    -- Delete events older than 90 days
    DELETE FROM logging_events WHERE created_at < NOW() - INTERVAL '90 days';

    -- Delete message logs older than 180 days
    DELETE FROM logging_messages WHERE created_at < NOW() - INTERVAL '180 days';

    -- Delete connection logs older than 30 days
    DELETE FROM logging_connections WHERE created_at < NOW() - INTERVAL '30 days';
END;
$$ LANGUAGE plpgsql;

-- Schedule cleanup (requires pg_cron extension)
-- SELECT cron.schedule('cleanup-old-logs', '0 2 * * *', 'SELECT cleanup_old_logs()');

-- Function to compute statistics
CREATE OR REPLACE FUNCTION compute_statistics(
    p_start TIMESTAMP,
    p_end TIMESTAMP
)
RETURNS void AS $$
BEGIN
    INSERT INTO logging_stats (
        period_start,
        period_end,
        total_events,
        total_messages,
        total_connections,
        messages_sent,
        messages_delivered,
        messages_failed,
        connections_success,
        connections_failed,
        errors_count,
        warnings_count
    )
    SELECT
        p_start,
        p_end,
        (SELECT COUNT(*) FROM logging_events WHERE created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_messages WHERE created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_connections WHERE created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_messages WHERE status = 'sent' AND created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_messages WHERE status = 'delivered' AND created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_messages WHERE status = 'failed' AND created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_connections WHERE success = true AND created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_connections WHERE success = false AND created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_events WHERE severity = 'error' AND created_at BETWEEN p_start AND p_end),
        (SELECT COUNT(*) FROM logging_events WHERE severity = 'warning' AND created_at BETWEEN p_start AND p_end)
    ON CONFLICT (period_start, period_end) DO UPDATE SET
        total_events = EXCLUDED.total_events,
        total_messages = EXCLUDED.total_messages,
        total_connections = EXCLUDED.total_connections,
        messages_sent = EXCLUDED.messages_sent,
        messages_delivered = EXCLUDED.messages_delivered,
        messages_failed = EXCLUDED.messages_failed,
        connections_success = EXCLUDED.connections_success,
        connections_failed = EXCLUDED.connections_failed,
        errors_count = EXCLUDED.errors_count,
        warnings_count = EXCLUDED.warnings_count,
        computed_at = NOW();
END;
$$ LANGUAGE plpgsql;

-- Comments
COMMENT ON TABLE logging_events IS 'General application events and logs';
COMMENT ON TABLE logging_messages IS 'Detailed message-specific logs with timing and metrics';
COMMENT ON TABLE logging_connections IS 'Network connection attempts and results';
COMMENT ON TABLE logging_stats IS 'Pre-computed statistics for performance';

COMMENT ON COLUMN logging_events.details IS 'Flexible JSONB field for additional event-specific data';
COMMENT ON COLUMN logging_events.client_timestamp IS 'Unix timestamp from client device';
COMMENT ON COLUMN logging_messages.message_id IS 'Reference to messages.id in main database';

-- Grant permissions (adjust user as needed)
-- GRANT SELECT, INSERT ON logging_events, logging_messages, logging_connections TO dna_messenger_app;
-- GRANT SELECT ON logging_stats TO dna_messenger_app;
-- GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO dna_messenger_app;
