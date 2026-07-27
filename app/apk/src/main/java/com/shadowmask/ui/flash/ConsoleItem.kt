package com.shadowmask.ui.flash

import com.shadowmask.R
import com.shadowmask.databinding.DiffItem
import com.shadowmask.databinding.ItemWrapper
import com.shadowmask.databinding.RvItem

class ConsoleItem(
    override val item: String
) : RvItem(), DiffItem<ConsoleItem>, ItemWrapper<String> {
    override val layoutRes = R.layout.item_console_md2
}
