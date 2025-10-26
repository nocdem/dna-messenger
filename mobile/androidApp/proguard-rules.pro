# DNA Messenger ProGuard Rules

# Keep native methods (JNI)
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep Kotlin Multiplatform shared module
-keep class io.cpunk.dna.** { *; }

# Keep data classes (for serialization)
-keep @kotlinx.serialization.Serializable class ** {
    *;
}

# Keep Compose
-keep class androidx.compose.** { *; }
-dontwarn androidx.compose.**

# Keep Coroutines
-keepnames class kotlinx.coroutines.internal.MainDispatcherFactory {}
-keepnames class kotlinx.coroutines.CoroutineExceptionHandler {}
-keepclassmembernames class kotlinx.** {
    volatile <fields>;
}

# Keep PostgreSQL JDBC
-keep class org.postgresql.** { *; }
-dontwarn org.postgresql.**

# Keep Biometric
-keep class androidx.biometric.** { *; }

# Remove logging in release
-assumenosideeffects class android.util.Log {
    public static *** d(...);
    public static *** v(...);
    public static *** i(...);
}

# Optimization
-optimizationpasses 5
-dontusemixedcaseclassnames
-dontskipnonpubliclibraryclasses
-verbose
