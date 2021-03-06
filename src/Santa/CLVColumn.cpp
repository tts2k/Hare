//Column list header source file

//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <string.h>
#include <Window.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "CLVColumn.h"
#include "ColumnListView.h"
#include "CLVColumnLabelView.h"
#include "NewStrings.h"


//******************************************************************************************************
//**** CLVColumn CLASS DEFINITION
//******************************************************************************************************
CLVColumn::CLVColumn(const char* label,float width,uint32 flags,float min_width) : BArchivable()
{
	Init(label,width,flags,min_width,NoSort);
}

CLVColumn::CLVColumn(BMessage* archive) : BArchivable(archive)
{
	status_t status = B_OK;
	const char* _label = NULL;
	float _width = 0.0;
	int32 _flags = 0;
	float _minWidth = 0.0;
	int32 _sortMode;

	status = archive->FindString("_label",&_label);
	if(status != B_OK)
	{
		_label = NULL;
	}

	status = archive->FindFloat("_minWidth",&_minWidth);
	if(status != B_OK)
	{
		_minWidth = 4.0;
	}

	status = archive->FindFloat("_width",&_width);
	if(status != B_OK)
	{
		_width = _minWidth;
	}

	status = archive->FindInt32("_flags",&_flags);
	if(status != B_OK)
	{
		_flags = 0;
	}

	status = archive->FindInt32("_sortMode",&_sortMode);
	if(status != B_OK)
	{
		_sortMode = NoSort;
	}

	Init(_label,_width,_flags,_minWidth,(CLVSortMode)_sortMode);
}

CLVColumn::~CLVColumn()
{
	if(fParent)
		fParent->RemoveColumn(this);
	if(fLabel)
		delete[] fLabel;
	if(fTruncatedText)
		delete[] fTruncatedText;
}

void CLVColumn::Init(const char* label,float width,uint32 flags,float min_width,CLVSortMode sortMode)
{
	if(flags & CLV_EXPANDER)
	{
		label = NULL;
		width = 20.0;
		min_width = 20.0;
		flags &= CLV_NOT_MOVABLE | CLV_LOCK_AT_BEGINNING | CLV_HIDDEN | CLV_LOCK_WITH_RIGHT;
		flags |= CLV_EXPANDER | CLV_NOT_RESIZABLE | CLV_MERGE_WITH_RIGHT;
	}
	if(min_width < 4.0)
		min_width = 4.0;
	if(width < min_width)
		width = min_width;
	if(label)
	{
		fLabel = new char[strlen(label)+1];
		strcpy((char*)fLabel,label);
		if(CLV_HEADER_TRUNCATE)
		{
			int32 truncated_text_length = strlen(label)+3;
			fTruncatedText = new char[truncated_text_length];
			fTruncatedText[0] = 0;
			fCachedRect.Set(-1,-1,-1,-1);		
		}
		else
			fTruncatedText = NULL;
	}
	else
	{
		fLabel = NULL;
		fTruncatedText = NULL;
	}
	fWidth = width;
	fMinWidth = min_width;
	fFlags = flags;
	fPushedByExpander = false;
	fParent = NULL;
	fSortMode = sortMode;
}

BArchivable* CLVColumn::Instantiate(BMessage* archive)
{
	BArchivable* result = NULL;

	if(validate_instantiation(archive,"CLVColumn"))
	{
		result = new CLVColumn(archive);
	}

	return result;
}

status_t CLVColumn::Archive(BMessage* archive, bool deep) const
{
	status_t status = B_OK;

	status = BArchivable::Archive(archive,deep);

	if(status == B_OK)
	{
		status = archive->AddString("_label",fLabel);
	}

	if(status == B_OK)
	{
		status = archive->AddFloat("_width",fWidth);
	}

	if(status == B_OK)
	{
		status = archive->AddFloat("_minWidth",fMinWidth);
	}

	if(status == B_OK)
	{
		status = archive->AddInt32("_flags",fFlags);
	}

	if(status == B_OK)
	{
		status = archive->AddInt32("_sortMode",fSortMode);
	}

	return status;
}

float CLVColumn::Width() const
{
	return fWidth;
}

void CLVColumn::SetWidth(float width)
{
	if(width < fMinWidth)
		width = fMinWidth;
	if(fParent && fParent->fEditTextView && fParent->fEditColumn == this &&
		fParent->fEditTextView->Frame().Width()-(width - fWidth) < 8)
	{
		width = fParent->fEditTextView->Frame().Width()-8+fWidth;
	}
	if(width != fWidth)
	{
		float OldWidth = fWidth;
		fWidth = width;
		if(IsShown() && fParent)
		{
			//Figure out the area after this column to scroll
			BRect ColumnViewBounds = fParent->fColumnLabelView->Bounds();
			BRect MainViewBounds = fParent->Bounds();
			BRect SourceArea = ColumnViewBounds;
			float Delta = width-OldWidth;
			if(!(fFlags&CLV_RIGHT_JUSTIFIED))
				SourceArea.left = fColumnEnd+1.0;
			else
			{
				if(Delta >= 0)
					SourceArea.left = fColumnBegin;
				else
					SourceArea.left = (fColumnBegin-Delta)+1;
			}
			BRect DestArea = SourceArea;
			DestArea.left += Delta;
			DestArea.right += Delta;
			float LimitShift;
			if(DestArea.right > ColumnViewBounds.right)
			{
				LimitShift = DestArea.right-ColumnViewBounds.right;
				DestArea.right -= LimitShift;
				SourceArea.right -= LimitShift;
			}
			if(DestArea.left < ColumnViewBounds.left)
			{
				LimitShift = ColumnViewBounds.left - DestArea.left;
				DestArea.left += LimitShift;
				SourceArea.left += LimitShift;
			}

			//Scroll the area that is being shifted
			BWindow* ParentWindow = fParent->Window();
			if(ParentWindow)
				ParentWindow->UpdateIfNeeded();
			fParent->fColumnLabelView->CopyBits(SourceArea,DestArea);
			SourceArea.top = MainViewBounds.top;
			SourceArea.bottom = MainViewBounds.bottom;
			DestArea.top = MainViewBounds.top;
			DestArea.bottom = MainViewBounds.bottom;
			fParent->CopyBits(SourceArea,DestArea);

			//Invalidate the region that got revealed
			DestArea = ColumnViewBounds;
			if(width > OldWidth)
			{
				if(!(fFlags&CLV_RIGHT_JUSTIFIED))
				{
					DestArea.left = fColumnEnd+1.0;
					DestArea.right = fColumnEnd+Delta;
				}
				else
				{
					DestArea.left = fColumnBegin;
					DestArea.right = fColumnBegin+Delta;
				}				
			}
			else
			{
				DestArea.left = ColumnViewBounds.right+Delta+1.0;
				DestArea.right = ColumnViewBounds.right;
			}
			fParent->fColumnLabelView->Invalidate(DestArea);
			DestArea.top = MainViewBounds.top;
			DestArea.bottom = MainViewBounds.bottom;
			fParent->Invalidate(DestArea);

			if(fFlags & CLV_HEADER_TRUNCATE)
			{
				//Do truncation of label
				BRect invalid_region = TruncateText(width);
				if(fCachedRect != BRect(-1,-1,-1,-1))
					fCachedRect.right += Delta;
				if(invalid_region != BRect(-1,-1,-1,-1))
				{
					if(!(fFlags&CLV_RIGHT_JUSTIFIED))
						GetHeaderView()->Invalidate(invalid_region);
					else
						GetHeaderView()->Invalidate(fCachedRect);
				}
			}

			//Invalidate the old or new resize handle as necessary
			DestArea = ColumnViewBounds;
			if(width > OldWidth)
				DestArea.left = fColumnEnd;
			else
				DestArea.left = fColumnEnd + Delta;
			DestArea.right = DestArea.left;
			fParent->fColumnLabelView->Invalidate(DestArea);
			
			//Update the column sizes, positions and group positions
			fParent->UpdateColumnSizesDataRectSizeScrollBars(false);
			fParent->fColumnLabelView->UpdateDragGroups();

			if(fParent->fEditTextView)
			{
				//Update the editing item if necessary
				BRect edit_text_view_frame = fParent->fEditTextView->Frame();
				if(fParent->fEditColumn == this)
				{
					fParent->fEditTextView->ResizeTo(edit_text_view_frame.Width()+(width-OldWidth),
						edit_text_view_frame.Height());
				}
				else if(fParent->IndexOfColumn(fParent->fEditColumn) > fParent->IndexOfColumn(this))
				{
					fParent->fEditTextView->MoveTo(edit_text_view_frame.left+(width-OldWidth),
						edit_text_view_frame.top);
				}
			}
		}
		if(fParent)
			fParent->ColumnWidthChanged(fParent->fColumnList.IndexOf(this),fWidth);
	}
}


BRect CLVColumn::TruncateText(float column_width)
//Returns whether the truncated text has changed
{
	column_width -= 1+8+5+1;
		//Because when I draw the text I start drawing 8 pixels to the right from the text area's left edge,
		//which is in turn 1 pixel smaller than the column at each edge, and I want 5 trailing pixels.
	BRect invalid(-1,-1,-1,-1);
	if(fParent == NULL)
		return invalid;
	const char* text = GetLabel();
	char new_text[256];
	BFont font;
	fParent->GetFont(&font);
	GetTruncatedString(text,new_text,column_width,256,&font);
	if(strcmp(fTruncatedText,new_text)!=0)
	{
		//The truncated text has changed
		invalid = fCachedRect;
		if(invalid != BRect(-1,-1,-1,-1))
		{
			//Figure out which region just got changed
			int32 cmppos;
			int32 cmplen = strlen(new_text);
			char remember = 0;
			for(cmppos = 0; cmppos <= cmplen; cmppos++)
				if(new_text[cmppos] != fTruncatedText[cmppos])
				{
					remember = new_text[cmppos];
					new_text[cmppos] = 0;
					break;
				}
			invalid.left += 8 + be_plain_font->StringWidth(new_text);
			new_text[cmppos] = remember;
		}
		//Remember the new truncated text
		strcpy(fTruncatedText,new_text);
	}
	return invalid;
}


void GetTruncatedString(const char* full_string, char* truncated, float width, int32 truncate_buf_size,
	const BFont* font)
{
	Strtcpy(truncated,full_string,truncate_buf_size);
	int32 choppos = strlen(truncated)-1;
	while(choppos >= -2 && font->StringWidth(truncated) > width)
	{
		while(choppos > 0 && truncated[choppos-1] == ' ')
			choppos--;
		if(choppos > 0 || (choppos == 0 && truncated[0] == ' '))
			truncated[choppos] = '.';
		if(choppos > -1)
			truncated[choppos+1] = '.';
		if(choppos > -2)
			truncated[choppos+2] = '.';
		truncated[choppos+3] = 0;
		choppos--;
	}
}


uint32 CLVColumn::Flags() const
{
	return fFlags;
}


bool CLVColumn::IsShown() const
{
	if(fFlags & CLV_HIDDEN)
		return false;
	else
		return true;
}


void CLVColumn::SetShown(bool Shown)
{
	//If there was editing going on, kill it
	if(fParent->fEditTextView)
		fParent->fEditTextView->DoneEditing(false);

	bool shown = IsShown();
	if(shown != Shown)
	{
		if(Shown)
			fFlags &= 0xFFFFFFFF^CLV_HIDDEN;
		else
			fFlags |= CLV_HIDDEN;
		if(fParent)
		{
			float UpdateLeft = fColumnBegin;
			fParent->UpdateColumnSizesDataRectSizeScrollBars();
			fParent->fColumnLabelView->UpdateDragGroups();
			if(Shown)
				UpdateLeft = fColumnBegin;
			BRect Area = fParent->fColumnLabelView->Bounds();
			Area.left = UpdateLeft;
			fParent->fColumnLabelView->Invalidate(Area);
			Area = fParent->Bounds();
			Area.left = UpdateLeft;
			fParent->Invalidate(Area);
			if(fFlags & CLV_EXPANDER)
			{
				if(!Shown)
					fParent->fExpanderColumn = -1;
				else
					fParent->fExpanderColumn = fParent->IndexOfColumn(this);
			}
		}
	}
}


CLVSortMode CLVColumn::SortMode() const
{
	return fSortMode;
}

void CLVColumn::SetSortMode(CLVSortMode mode)
{
	if(fParent)
		fParent->SetSortMode(fParent->IndexOfColumn(this),mode);
	else
		fSortMode = mode;
}


const char* CLVColumn::GetLabel() const
{
	return fLabel;
}


void CLVColumn::SetLabel(const char *label)
{
	fParent->fColumnLabelView->Invalidate(fCachedRect);
	if(fLabel)
		delete[] fLabel;
	fLabel = NULL;
	if(fTruncatedText)
		delete[] fTruncatedText;
	fTruncatedText = NULL;
	if(label)
	{
		fLabel = new char[strlen(label)+1];
		strcpy((char*)fLabel,label);
		if(CLV_HEADER_TRUNCATE)
		{
			int32 truncated_text_length = strlen(label)+3;
			fTruncatedText = new char[truncated_text_length];
			fTruncatedText[0] = 0;
			fCachedRect.Set(-1,-1,-1,-1);		
		}
	}
}


ColumnListView* CLVColumn::GetParent() const
{
	return fParent;
}


BView* CLVColumn::GetHeaderView() const
{
	if(fParent)
		return fParent->fColumnLabelView;
	else
		return NULL;
}


void CLVColumn::DrawColumnHeader(BView* view, BRect header_rect, bool sort_key, bool focus,
	float font_ascent)
{
	char* label;
	if(fFlags & CLV_HEADER_TRUNCATE)
	{
		if(fCachedRect == BRect(-1,-1,-1,-1))
		{
			//Have never drawn it before
			TruncateText(header_rect.right-header_rect.left);
		}
		label = fTruncatedText;
	}
	else
		label = fLabel;

	if(label)
	{
		if(focus)
			view->SetHighColor(BeFocusBlue);
		else
			view->SetHighColor(Black);
	
		//Draw the label
		view->SetDrawingMode(B_OP_OVER);
		BPoint text_point;
		if(!(fFlags&CLV_RIGHT_JUSTIFIED))
			text_point.Set(header_rect.left+8.0,header_rect.top+1.0+font_ascent);
		else
		{
			BFont label_font;
			view->GetFont(&label_font);
			float string_width = label_font.StringWidth(label);
			text_point.Set(header_rect.right-8.0-string_width,header_rect.top+1.0+font_ascent);			
		}
		view->DrawString(label,text_point);
		view->SetDrawingMode(B_OP_COPY);
	
		//Underline if this is a selected sort column
		if(sort_key)
		{
			float Width = view->StringWidth(label);
			view->StrokeLine(BPoint(text_point.x-1,text_point.y+2.0),
				BPoint(text_point.x-1+Width,text_point.y+2.0));
		}
		fCachedRect = header_rect;
	}
}

