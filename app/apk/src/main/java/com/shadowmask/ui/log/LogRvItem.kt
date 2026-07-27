package com.shadowmask.ui.log

import com.shadowmask.R
import com.shadowmask.databinding.DiffItem
import com.shadowmask.databinding.ItemWrapper
import com.shadowmask.databinding.ObservableRvItem

class LogRvItem(
    override val item: String
) : ObservableRvItem(), DiffItem<LogRvItem>, ItemWrapper<String> {
    override val layoutRes = R.layout.item_log_textview
}
