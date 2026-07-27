package com.shadowmask.ui.theme

import com.shadowmask.arch.BaseViewModel
import com.shadowmask.core.Config
import com.shadowmask.dialog.DarkThemeDialog
import com.shadowmask.events.RecreateEvent
import com.shadowmask.view.TappableHeadlineItem

class ThemeViewModel : BaseViewModel(), TappableHeadlineItem.Listener {

    val themeHeadline = TappableHeadlineItem.ThemeMode

    override fun onItemPressed(item: TappableHeadlineItem) = when (item) {
        is TappableHeadlineItem.ThemeMode -> DarkThemeDialog().show()
    }

    fun saveTheme(theme: Theme) {
        if (!theme.isSelected) {
            Config.themeOrdinal = theme.ordinal
            RecreateEvent().publish()
        }
    }
}
