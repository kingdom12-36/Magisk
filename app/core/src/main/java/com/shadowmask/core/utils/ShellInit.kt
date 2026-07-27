package com.shadowmask.core.utils

import android.content.Context
import com.shadowmask.StubApk
import com.shadowmask.core.Const
import com.shadowmask.core.Info
import com.shadowmask.core.isRunningAsStub
import com.shadowmask.core.ktx.cachedFile
import com.shadowmask.core.ktx.deviceProtectedContext
import com.shadowmask.core.ktx.writeTo
import com.topjohnwu.superuser.Shell
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import java.io.File
import java.util.jar.JarFile

class ShellInit : Shell.Initializer() {
    override fun onInit(context: Context, shell: Shell): Boolean {
        if (shell.isRoot) {
            Info.isRooted = true
            RootUtils.bindTask?.let { shell.execTask(it) }
            RootUtils.bindTask = null
        }
        shell.newJob().apply {
            add("export ASH_STANDALONE=1")

            val localBB: File
            if (isRunningAsStub) {
                if (!shell.isRoot)
                    return true
                val jar = JarFile(StubApk.current(context))
                val bb = jar.getJarEntry("lib/${Const.CPU_ABI}/libbusybox.so")
                localBB = context.deviceProtectedContext.cachedFile("busybox")
                localBB.delete()
                runBlocking {
                    jar.getInputStream(bb).writeTo(localBB, dispatcher = Dispatchers.Unconfined)
                }
                localBB.setExecutable(true)
            } else {
                localBB = File(context.applicationInfo.nativeLibraryDir, "libbusybox.so")
            }

            if (shell.isRoot) {
                add("export MAGISKTMP=\$(magisk --path)")
                // Test if we can properly execute stuff in /data
                Info.noDataExec = !shell.newJob()
                    .add("$localBB sh -c '$localBB true'").exec().isSuccess
            }

            if (Info.noDataExec) {
                // Copy it out of /data to workaround Samsung bullshit
                add(
                    "if [ -x \$MAGISKTMP/.shadowmask/busybox/busybox ]; then",
                    "  cp -af $localBB \$MAGISKTMP/.shadowmask/busybox/busybox",
                    "  exec \$MAGISKTMP/.shadowmask/busybox/busybox sh",
                    "else",
                    "  cp -af $localBB /dev/busybox",
                    "  exec /dev/busybox sh",
                    "fi"
                )
            } else {
                // Directly execute the file
                add("exec $localBB sh")
            }

            add(context.assets.open("app_functions.sh"))
            if (shell.isRoot) {
                add(context.assets.open("util_functions.sh"))
            }
        }.exec()

        Info.init(shell)
        return true
    }
}
