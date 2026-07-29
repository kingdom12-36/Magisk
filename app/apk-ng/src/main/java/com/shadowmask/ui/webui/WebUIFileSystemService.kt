package com.shadowmask.ui.webui

import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import androidx.annotation.MainThread
import com.shadowmask.core.AppContext
import com.topjohnwu.superuser.Shell
import com.topjohnwu.superuser.ipc.RootService
import com.topjohnwu.superuser.nio.FileSystemManager
import java.util.concurrent.CopyOnWriteArraySet

class WebUIFileSystemService : RootService() {
    override fun onBind(intent: Intent): IBinder {
        return FileSystemManager.getService()
    }

    interface Listener {
        fun onServiceAvailable(fs: FileSystemManager)
        fun onLaunchFailed()
    }

    companion object {
        private sealed class Status {
            data object Uninitialized : Status()
            data object CheckRoot : Status()
            data class ServiceAvailable(val fs: FileSystemManager) : Status()
        }

        private var status: Status = Status.Uninitialized
        private val uiHandler = Handler(Looper.getMainLooper())

        private val connection = object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName, binder: IBinder) {
                val fs = FileSystemManager.getRemote(binder)
                status = Status.ServiceAvailable(fs)
                val snapshot = pendingListeners.toList()
                pendingListeners.clear()
                snapshot.forEach { it.onServiceAvailable(fs) }
            }

            override fun onServiceDisconnected(name: ComponentName) {
                status = Status.Uninitialized
            }
        }

        private val pendingListeners = CopyOnWriteArraySet<Listener>()

        @MainThread
        fun start(listener: Listener) {
            (status as? Status.ServiceAvailable)?.let {
                listener.onServiceAvailable(it.fs)
                return
            }
            pendingListeners.add(listener)
            if (status == Status.Uninitialized) {
                checkRoot()
            }
        }

        private fun checkRoot() {
            status = Status.CheckRoot
            Thread {
                val isRoot = Shell.Builder.create()
                    .setFlags(Shell.FLAG_MOUNT_MASTER)
                    .build()
                    .use { it.isRoot }
                uiHandler.post {
                    if (isRoot) {
                        launchService()
                    } else {
                        status = Status.Uninitialized
                        val snapshot = pendingListeners.toList()
                        pendingListeners.clear()
                        snapshot.forEach { it.onLaunchFailed() }
                    }
                }
            }.start()
        }

        private fun launchService() {
            bind(Intent(AppContext, WebUIFileSystemService::class.java), connection)
        }

        fun removeListener(listener: Listener) {
            pendingListeners.remove(listener)
        }
    }
}
