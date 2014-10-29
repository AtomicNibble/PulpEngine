

const char*	XGui::getName(void) const
{
	return name_.c_str();
}

void XGui::setCursorPos(float x, float y)
{
	cursorPos_.x = x;
	cursorPos_.y = y;
}

void XGui::setCursorPos(const Vec2f& pos)
{
	cursorPos_ = pos;
}

Vec2f XGui::getCursorPos(void)
{
	return cursorPos_;
}

float XGui::getCursorPosX(void)
{
	return cursorPos_.x;
}

float XGui::getCursorPosY(void)
{
	return cursorPos_.y;
}


// ------------------------

bool XGui::isDeskTopValid(void) const
{
	return pDesktop_ != nullptr;
}