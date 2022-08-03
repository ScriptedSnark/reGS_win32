#include "BackgroundMenuButton.h"

CBackgroundMenuButton::CBackgroundMenuButton(vgui2::Panel* parent, const char* name)
	: BaseClass(parent, name, "")
{
}

CBackgroundMenuButton::~CBackgroundMenuButton()
{
}

void CBackgroundMenuButton::ApplySchemeSettings(vgui2::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(Color(255, 255, 255, 255));
	SetBgColor(Color(0, 0, 0, 0));

	SetDefaultColor(Color(255, 255, 255, 255), Color(0, 0, 0, 0));
	SetArmedColor(Color(255, 0, 255, 255), Color(0, 0, 0, 0));
	SetDepressedColor(Color(255, 0, 255, 255), Color(0, 0, 0, 0));

	SetContentAlignment(vgui2::Label::a_west);

	SetBorder(nullptr);
	SetDefaultBorder(nullptr);
	SetDepressedBorder(nullptr);
	SetKeyFocusBorder(nullptr);
	SetTextInset(0, 0);

	SetArmedSound("sound/UI/buttonrollover.wav");
	SetDepressedSound("sound/UI/buttonclick.wav");
	SetReleasedSound("sound/UI/buttonclickrelease.wav");
}

void CBackgroundMenuButton::OnCommand(const char* command)
{
	BaseClass::OnCommand(command);
}
