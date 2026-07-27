plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.shadowmask.test"

    defaultConfig {
        applicationId = "com.shadowmask.test"
        versionCode = 1
        versionName = "1.0"
        proguardFile("proguard-rules.pro")
    }

    buildTypes {
        release {
            isMinifyEnabled = true
        }
    }
}

setupTestApk()

dependencies {
    implementation(libs.test.runner)
    implementation(libs.test.rules)
    implementation(libs.test.junit)
    implementation(libs.test.uiautomator)
}
