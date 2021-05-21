void minigame();
void travel();
int convertAccelToMovement();
void travelStatus();
void minigameStatus();
void calcFrameRate();
void newGame();
void titleScreen();
void drawShip(int x, int y, bool isKrait);
void spatialAnomaly();
bool awaitInput(bool sound);
void eventText(const char *text);
void choiceText(const char *desc, const char *lChoice1, const char *lChoice2, const char *rChoice1, const char *rChoice2);
void thargoidEncounter();
void gameOver();
void menu();
void waypoint(int index);
void refuel();
void statusAlert();
void victory();

class Ship : public Rectangle
{
protected:
    bool isKrait;

public:
    Ship(int x, int y, int width, int height) : Rectangle(x, y, width, height)
    {
        isKrait = false;
    }

    bool getIsKrait()
    {
        return isKrait;
    }

    void setKrait(bool val)
    {
        isKrait = val;
    }
};

class Asteroid : public Circle
{
protected:
    bool hasPassedShip = false;
    bool display = false;

public:
    Asteroid(int x, int y, int radius, bool display) : Circle(x, y, radius)
    {
        display = display;
        setDrawFill(true);
    }

    bool getHasPassed()
    {
        return hasPassedShip;
    }

    void setHasPassed(bool newVal)
    {
        hasPassedShip = newVal;
    }

    bool getDisplay()
    {
        return display;
    }

    void setDisplay(bool newVal)
    {
        display = newVal;
    }
};
