plugins {
    alias(libs.plugins.android.application)
}

setupCommon()

android {
    namespace = "com.shadowmask"
    enableKotlin = false

    buildTypes {
        release {
            isShrinkResources = false
        }
    }
}
