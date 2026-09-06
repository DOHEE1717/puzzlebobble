#pragma once
#include "Hedder.h"
#include "Bubble.h"

class BubbleGrid
{
public:
	BubbleGrid(const SDL_Rect& playArea, int bubbleSize, int maxRows, int maxCols);

	bool WorldToGrid(float x, float y, int& outRow, int& outCol) const;
	void GridToWorld(int row, int col, float& outX, float& outY)const;
	bool IsValidCell(int row, int col) const;
	bool IsEmpty(int row, int col) const;
	void AddBubble(int row, int col, int color);
	PlacedBubble* FindBubble(int row, int col);
	const std::vector<PlacedBubble>& GetAll()const { return bubbles; }
	int RemoveCluster(int startRow, int startCol, int minMatch);
	int RemoveConnectedRegionAnyColor(int startRow, int startCol);
	int RemoveRowAll(int targetRow);
	int RemoveRadius(int centerRow, int centerCol, int radius = 1);
	bool FindNearestEmptyCell(float worldX, float worldY, int& outRow, int& outCol)const;
	void SetBlockedTopRows(int rows) { blockedTopRows = rows; }
	void SetRowOffset(int offset);
	int RemoveFloatingBubbles();

	bool HasBubbleRow(int row) const;
	const std::vector<PlacedBubble>& GetLastRemovedCluster() const { return lastRemovedCluster; }
	const std::vector<PlacedBubble>& GetLastRemovedFloating() const { return lastRemovedFloating; }


protected:

private:
	SDL_Rect playArea;
	int bubbleSize;
	int maxRows;
	int maxCols;
	int blockedTopRows = 0;
	int rowOffset = 0;

	std::vector<PlacedBubble> bubbles;

	std::vector<PlacedBubble> lastRemovedCluster;
	std::vector<PlacedBubble> lastRemovedFloating;
};

