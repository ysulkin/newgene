#include "kadwidgetsscrollarea.h"

#include "kadspinbox.h"
#include <QLayout>
#include <QFont>
#include <QGraphicsColorizeEffect>
#include <QLabel>
#include <QSize>
#include <QPalette>
#include <QPainter>
#include <QSpacerItem>

#include "../Project/uiprojectmanager.h"
#include "../Project/uiinputproject.h"
#include "../Project/uioutputproject.h"

#include "../Variables/newgenevariablestoolboxwrapper.h"
#include "../Variables/newgenevariablestoolbox.h"

KadWidgetsScrollArea::KadWidgetsScrollArea(QWidget * parent) :
	QWidget(parent),
	NewGeneWidget(WidgetCreationInfo(this, parent, WIDGET_NATURE_OUTPUT_WIDGET, KAD_SPIN_CONTROLS_AREA,
									 true))   // 'this' pointer is cast by compiler to proper Widget instance, which is already created due to order in which base classes appear in class definition
{

	QBoxLayout * layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
	layout->addStretch();
	layout->addStretch();
	setLayout(layout);

	loading = false;

	PrepareOutputWidget();
}

void KadWidgetsScrollArea::UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project)
{
	NewGeneWidget::UpdateOutputConnections(connection_type, project);

	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT)
	{

		connect(this, SIGNAL(RefreshWidget(WidgetDataItemRequest_KAD_SPIN_CONTROLS_AREA)), outp->getConnector(), SLOT(RefreshWidget(WidgetDataItemRequest_KAD_SPIN_CONTROLS_AREA)));

		// *** This is a parent (top-level) widget, so connect to refreshes here (... child widgets don't connect to refreshes) *** //
		connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_KAD_SPIN_CONTROLS_AREA)), this, SLOT(WidgetDataRefreshReceive(WidgetDataItem_KAD_SPIN_CONTROLS_AREA)));

		// *** Has child widgets, so refer refresh signals directed at child to be received by us, the parent *** //
		connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_KAD_SPIN_CONTROL_WIDGET)), this, SLOT(WidgetDataRefreshReceive(WidgetDataItem_KAD_SPIN_CONTROL_WIDGET)));

	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_OUTPUT_PROJECT)
	{

		project->UnregisterInterestInChanges(this);
		Empty();

	}
}

void KadWidgetsScrollArea::UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project)
{
	NewGeneWidget::UpdateInputConnections(connection_type, project);

	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_INPUT_PROJECT)
	{
		if (project)
		{
			project->RegisterInterestInChange(this, DATA_CHANGE_TYPE__INPUT_MODEL__DMU_CHANGE, false, "");
		}
	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_INPUT_PROJECT)
	{
		if (inp)
		{
			inp->UnregisterInterestInChanges(this);
		}
	}
}

void KadWidgetsScrollArea::RefreshAllWidgets()
{
	if (outp == nullptr)
	{
		Empty();
		return;
	}

	WidgetDataItemRequest_KAD_SPIN_CONTROLS_AREA request(WIDGET_DATA_ITEM_REQUEST_REASON__REFRESH_ALL_WIDGETS);
	emit RefreshWidget(request);
}

void KadWidgetsScrollArea::WidgetDataRefreshReceive(WidgetDataItem_KAD_SPIN_CONTROLS_AREA widget_data)
{

	Empty();

	std::for_each(widget_data.identifiers.cbegin(), widget_data.identifiers.cend(), [&](WidgetInstanceIdentifier const & identifier)
	{
		if (identifier.uuid && identifier.code && identifier.longhand)
		{
			WidgetInstanceIdentifiers active_dmus(widget_data.active_dmus.cbegin(), widget_data.active_dmus.cend());
			AddKadSpinWidget(identifier, active_dmus);
		}
	});

	EmptyTextCheck();

}

void KadWidgetsScrollArea::WidgetDataRefreshReceive(WidgetDataItem_KAD_SPIN_CONTROL_WIDGET widget_data)
{
	if (widget_data.identifier && widget_data.identifier->uuid)
	{
		NewGeneWidget * child = outp->FindWidgetFromDataItem(uuid, *widget_data.identifier->uuid);

		if (child)
		{
			child->WidgetDataRefreshReceive(widget_data);
		}
	}
}

void KadWidgetsScrollArea::Empty()
{
	QLayoutItem * child;

	while ((child = layout()->takeAt(0)) != 0)
	{
		delete child->widget();
		delete child;
	}

	try
	{
		QBoxLayout * boxLayout = dynamic_cast<QBoxLayout *>(layout());

		if (boxLayout)
		{
			boxLayout->addStretch();
			boxLayout->addStretch();
		}
	}
	catch (std::bad_cast &)
	{
	}

	EmptyTextCheck();
}

void KadWidgetsScrollArea::HandleChanges(DataChangeMessage const & change_message)
{

	UIOutputProject * project = projectManagerUI().getActiveUIOutputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	std::for_each(change_message.changes.cbegin(), change_message.changes.cend(), [this](DataChange const & change)
	{
		switch (change.change_type)
		{
			case DATA_CHANGE_TYPE::DATA_CHANGE_TYPE__INPUT_MODEL__DMU_CHANGE:
				{
					switch (change.change_intention)
					{
						case DATA_CHANGE_INTENTION__ADD:
							{
								if (change.parent_identifier.code && change.parent_identifier.uuid)
								{
									/* From the server arrives a list of DMU INSTANCES (USA, Russia, etc.), not DMUs (COUNTRY, etc.) -
									this is NOT what the function AddKadSpinWidget expects, so send empty for the second argument.
									HOWEVER, this line is only reached when the user has added a NEW DMU so the list is empty anyways;
									note that when this function is called, the new DMU is HIDDEN, which is as it should be,
									because it should only be shown when there are variables selected corresponding to VGs
									corresponding to UOAs corresponding to this new DMU, and there can't be when the DMU was just created,
									so the new DMU's Kad spin box should certainly always be hidden in this scenario */
									AddKadSpinWidget(change.parent_identifier, WidgetInstanceIdentifiers());
								}
							}
							break;

						case DATA_CHANGE_INTENTION__REMOVE:
							{

								if (change.parent_identifier.code && change.parent_identifier.uuid)
								{

									WidgetInstanceIdentifier uoa_to_remove(change.parent_identifier);

									int current_number = layout()->count();
									bool found = false;
									QWidget * widgetToRemove = nullptr;
									QLayoutItem * layoutItemToRemove = nullptr;
									int i = 0;

									for (i = 0; i < current_number; ++i)
									{
										QLayoutItem * testLayoutItem = layout()->itemAt(i);
										QWidget * testWidget(testLayoutItem->widget());

										try
										{
											KadSpinBox * testSpinBox = dynamic_cast<KadSpinBox *>(testWidget);

											if (testSpinBox && testSpinBox->data_instance.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, uoa_to_remove))
											{
												widgetToRemove = testSpinBox;
												layoutItemToRemove = testLayoutItem;
												found = true;
												break;
											}
										}
										catch (std::bad_cast &)
										{
											// guess not
										}

									}

									if (found && widgetToRemove != nullptr)
									{
										layout()->takeAt(i);
										delete widgetToRemove;
										delete layoutItemToRemove;
										widgetToRemove = nullptr;
										layoutItemToRemove = nullptr;

										EmptyTextCheck();

										Resequence();
									}

								}

							}
							break;

						case DATA_CHANGE_INTENTION__UPDATE:
							{
								// Should never receive this.
							}

						case DATA_CHANGE_INTENTION__RESET_ALL:
							{
								// Ditto above.
							}
							break;

						default:
							{
							}
							break;
					}
				}
				break;

			default:
				{
				}
				break;
		}
	});

}

void KadWidgetsScrollArea::AddKadSpinWidget(WidgetInstanceIdentifier const & identifier, WidgetInstanceIdentifiers const & active_dmus)
{

	// Only called when an actual DMU is being ADDED to the input dataset!!
	// (or on full refresh)

	// Remove the stretch at the end
	QLayoutItem * stretcher {};

	if (layout()->count() && (stretcher = layout()->takeAt(layout()->count() - 1)) != 0)
	{
		try
		{
			QSpacerItem * spacer { dynamic_cast<QSpacerItem *>(stretcher) };

			if (spacer)
			{
				delete stretcher->widget();
				delete stretcher;
			}
		}
		catch (std::bad_cast &)
		{

		}
	}

	WidgetInstanceIdentifier new_identifier(identifier);
	KadSpinBox * newSpinBox = new KadSpinBox(this, new_identifier, outp);
	newSpinBox->setFixedHeight(20);
	newSpinBox->setFixedWidth(200);
	QFont currFont = newSpinBox->font();
	currFont.setPixelSize(11);
	newSpinBox->setFont(currFont);
	std::string stylesheet(" QSpinBox { color: #333333; font-weight: bold; } ");
	newSpinBox->setStyleSheet(stylesheet.c_str());
	std::string prefixText(" #");
	prefixText += *identifier.longhand;
	prefixText += " cols:  ";
	newSpinBox->setPrefix(prefixText.c_str());
	bool not_me = true;
	std::for_each(active_dmus.cbegin(), active_dmus.cend(), [&](WidgetInstanceIdentifier const & the_dmu)
	{
		if (identifier.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__STRING_CODE, the_dmu))
		{
			not_me = false;
		}
	});

	// DMUs for ALL UOAs corresponding to ALL variable groups EXIST,
	// but only those corresponding to UOAs corresponding to variable groups with SELECTED variables
	// are VISIBLE.
	if (not_me)
	{
		newSpinBox->doSetVisible(false);
	}
	else
	{
		newSpinBox->doSetVisible(true);
	}

	layout()->addWidget(newSpinBox);

	try
	{
		QBoxLayout * boxLayout = dynamic_cast<QBoxLayout *>(layout());

		if (boxLayout)
		{
			boxLayout->addStretch();
		}
	}
	catch (std::bad_cast &)
	{
	}

	EmptyTextCheck();

	Resequence();
}

void KadWidgetsScrollArea::ShowLoading(bool const loading_)
{
	loading = loading_;
	EmptyTextCheck();
}

void KadWidgetsScrollArea::EmptyTextCheck()
{

	QLabel * emptySpinsLabel { findChild<QLabel *>("emptyKadsLabel") };
	QLabel * noOutputProjectLabel { findChild<QLabel *>("noOutputProjectLabel") };
	QFrame * frameLoadingDataset { findChild<QFrame *>("frameLoadingDataset") };

	if (loading)
	{
		if (emptySpinsLabel)
		{
			emptySpinsLabel->setVisible(false);
		}

		if (noOutputProjectLabel)
		{
			noOutputProjectLabel->setVisible(false);
		}

		if (frameLoadingDataset)
		{
			frameLoadingDataset->setVisible(true);
		}

		return;
	}

	if (frameLoadingDataset)
	{
		frameLoadingDataset->setVisible(false);
	}

	int current_number = layout()->count();
	bool any_spincontrols_visible = false;
	int i = 0;

	for (i = 0; i < current_number; ++i)
	{
		QLayoutItem * testLayoutItem = layout()->itemAt(i);
		QWidget * testWidget(testLayoutItem->widget());

		try
		{
			KadSpinBox * testSpinBox = dynamic_cast<KadSpinBox *>(testWidget);

			if (testSpinBox && testSpinBox->visible)
			{
				any_spincontrols_visible = true;
				break;
			}
		}
		catch (std::bad_cast &)
		{
			// this will catch throws for the "stretch" layout items, but who cares - it's infrequent
		}

	}

	bool noOutputProjectOpen = (outp == nullptr ? true : false);

	if (!noOutputProjectOpen)
	{
		if (emptySpinsLabel)
		{
			if (!any_spincontrols_visible)
			{
				emptySpinsLabel->setVisible(true);
			}
			else
			{
				emptySpinsLabel->setVisible(false);
			}
		}

		if (noOutputProjectLabel)
		{
			noOutputProjectLabel->setVisible(false);
		}
	}
	else
	{
		if (emptySpinsLabel)
		{
			emptySpinsLabel->setVisible(false);
		}

		if (noOutputProjectLabel)
		{
			noOutputProjectLabel->setVisible(true);
		}
	}

}

void KadWidgetsScrollArea::resizeEvent(QResizeEvent *)
{
	QLabel * emptySpinsLabel { findChild<QLabel *>("emptyKadsLabel") };

	if (emptySpinsLabel)
	{
		QSize mySize { size() };
		QSize labelSize { emptySpinsLabel->size() };
		emptySpinsLabel->move(mySize.width() / 2 - labelSize.width() / 2, mySize.height() / 2 - labelSize.height() / 2);
	}

	QLabel * noOutputProjectLabel { findChild<QLabel *>("noOutputProjectLabel") };

	if (noOutputProjectLabel)
	{
		QSize mySize { size() };
		QSize labelSize { noOutputProjectLabel->size() };
		noOutputProjectLabel->move(mySize.width() / 2 - labelSize.width() / 2, mySize.height() / 2 - labelSize.height() / 2);
	}

	QFrame * frameLoadingDataset { findChild<QFrame *>("frameLoadingDataset") };

	if (frameLoadingDataset)
	{
		QSize mySize { size() };
		QSize frameSize { frameLoadingDataset->size() };
		frameLoadingDataset->move(mySize.width() / 2 - frameSize.width() / 2, mySize.height() / 2 - frameSize.height() / 2);
	}

	EmptyTextCheck();
}

void KadWidgetsScrollArea::paintEvent(QPaintEvent *)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void KadWidgetsScrollArea::Resequence()
{
	NewGeneVariablesToolboxWrapper * toolbox = this->parentWidget()->parentWidget()->parentWidget()->parentWidget()->findChild<NewGeneVariablesToolboxWrapper *>("toolbox");

	if (toolbox)
	{
		WidgetInstanceIdentifiers orderedDmus = toolbox->getDmuSequence();
		bool misordered = false;

		{
			int order = 0;

			for (auto orderedDmu : orderedDmus)
			{
				int current_number = layout()->count();

				for (int i = 0; i < current_number; ++i)
				{
					QLayoutItem * testLayoutItem = layout()->itemAt(i);
					QWidget * testWidget(testLayoutItem->widget());

					try
					{
						KadSpinBox * testSpinBox = dynamic_cast<KadSpinBox *>(testWidget);

						if (testSpinBox)
						{
							if (orderedDmu.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__STRING_CODE, testSpinBox->data_instance))
							{
								if (order != i)
								{
									misordered = true;
									break;
								}
							}
						}
					}
					catch (std::bad_cast &)
					{
						// this will catch throws for the "stretch" layout items, but who cares - it's infrequent
					}
				}

				if (misordered)
				{
					break;
				}

				++order;
			}
		}

		if (misordered)
		{
			// Remove the stretch at the end
			QLayoutItem * stretcher {};

			if (layout()->count() && (stretcher = layout()->takeAt(layout()->count() - 1)) != 0)
			{
				try
				{
					QSpacerItem * spacer { dynamic_cast<QSpacerItem *>(stretcher) };

					if (spacer)
					{
						delete stretcher->widget();
						delete stretcher;
					}
				}
				catch (std::bad_cast &)
				{

				}
			}

			std::vector<KadSpinBox *> cache;

			for (auto orderedDmu : orderedDmus)
			{
				int current_number = layout()->count();

				for (int i = 0; i < current_number; ++i)
				{
					QLayoutItem * testLayoutItem = layout()->itemAt(i);
					QWidget * testWidget(testLayoutItem->widget());

					try
					{
						KadSpinBox * testSpinBox = dynamic_cast<KadSpinBox *>(testWidget);

						if (testSpinBox)
						{
							if (orderedDmu.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__STRING_CODE, testSpinBox->data_instance))
							{
								cache.push_back(testSpinBox);
								layout()->removeWidget(testSpinBox);
								break;
							}
						}
					}
					catch (std::bad_cast &)
					{
						// this will catch throws for the "stretch" layout items, but who cares - it's infrequent
					}
				}
			}


			for (auto cachedWidget : cache)
			{
				layout()->addWidget(cachedWidget);
			}

			try
			{
				QBoxLayout * boxLayout = dynamic_cast<QBoxLayout *>(layout());

				if (boxLayout)
				{
					boxLayout->addStretch();
				}
			}
			catch (std::bad_cast &)
			{
			}
		}
	}
}
