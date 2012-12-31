#ifndef CONTROLVALUEMESSAGE_H
#define CONTROLVALUEMESSAGE_H

namespace phui
{
class View;
class Message;

/************************************************************************************************
	ViewMessage
*************************************************************************************************/
// These functions set and get the values of control from within the view and stuff them in a Message object

void RestoreControlValues(phui::View*, phui::Message const*);
void ArchiveControlValues(phui::View const*, phui::Message*);

}

#endif // CONTROLVALUEMESSAGE_H
