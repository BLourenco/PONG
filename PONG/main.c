/*
	Program: PONG
	Author: Brandon Lourenco
	Date: January 23, 2015
*/

#include <SDL.h>    // include SDL stuff
#include <stdio.h>  // standard input/output
#include <time.h>   // used for rng
#include <stdlib.h> // contains rand()

int main(int argc, char** argv)
{
	//
	// declare variables
	//
	SDL_Window* window = NULL;      // a window to draw stuff on
	const Uint8* keys = NULL;       // pointer to keyboard state managed by SDL
	SDL_Renderer* renderer = NULL;  // processes our drawing commands
	SDL_Rect p1;					
	SDL_Rect p2;
	SDL_Rect ball;

	SDL_Point p1Center;				// used in collision detection
	SDL_Point p2Center;
	SDL_Point ballCenter;			

	srand(time(NULL));				// random number seed

	// flags
	int done = 0;                   // set this to a non-zero value to exit the main loop
	int ballInPlay = 0;				// set this to a non-zero value when ball has been served, revert when a point is scored
	int gameOn = 0;					// set this to a non-zero value when a game has started, revert when a player wins
	int RWGMode = 0;				// set this to a non-zero value to play Red, White, Green mode
	int multiplayer = 0;			// set this to a non-zero value to play against another human player

	// toggles: -1 = OFF; 1 = ON
	int scanlines = 1;				// invert to show/hide scanlines

	// key locks: prevents firing per frame
	int helpLock = 0;
	int scanlinesLock = 0;
	int p1AColorSwitchLock = 0;
	int p1DColorSwitchLock = 0;
	int p2LColorSwitchLock = 0;
	int p2RColorSwitchLock = 0;

	int p1Score = 0;
	int p2Score = 0;
	int winScore = 11;

	char title[6];
	sprintf(title, "%d-%d", p1Score, p2Score);
	

	/* Score Display Digit Pieces

		    _0_ 
		  1|   |2
		   |_3_|  
		  4|   |5
		   |_6_| 
	*/

	int scoreDisplay[10][7] = {
		//0  1  2  3  4  5  6	<- Pieces
		{ 1, 1, 1, 0, 1, 1, 1 }, // 0
		{ 0, 0, 1, 0, 0, 1, 0 }, // 1
		{ 1, 0, 1, 1, 1, 0, 1 }, // 2
		{ 1, 0, 1, 1, 0, 1, 1 }, // 3
		{ 0, 1, 1, 1, 0, 1, 0 }, // 4
		{ 1, 1, 0, 1, 0, 1, 1 }, // 5
		{ 1, 1, 0, 1, 1, 1, 1 }, // 6
		{ 1, 0, 1, 0, 0, 1, 0 }, // 7
		{ 1, 1, 1, 1, 1, 1, 1 }, // 8
		{ 1, 1, 1, 1, 0, 1, 1 }  // 9
	};							 // ^ Digits

	SDL_Color *p1Color;
	SDL_Color *p2Color;
	SDL_Color *ballColor;

	int p1ColorSetting = 0;
	int p2ColorSetting = 0;
	int ballColorSetting = 0;

	SDL_Color white;	// color 0
	SDL_Color red;		// color 1
	SDL_Color green;	// color 2

	int scrWidth = 640;
	int scrHeight = 480;

	int paddleSpeed = 7;

	int ballSpeedX = 0;
	int ballSpeedY = 0;
	
	int ballDirX = 1;

	int ballSpeedCapX = 6;
	int ballSpeedCapY = 10;

	int aiMovement = 0;		// 0 = stationary; 1 = down; -1 = up

	int ballHits = 0;	
	int hitsPerSpeedUp = 1; // Every X hits, increase ballSpeedX
	int lastPoint = 0;	// set to 1 when P1 scores, and 2 when P2 scores. Determines serve direction

	//
	// initialize SDL
	//
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "*** Failed to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	//
	// create a window
	//
	window = SDL_CreateWindow(title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		scrWidth, scrHeight,
		SDL_WINDOW_SHOWN); // Removed resizable window: "SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE"
	if (!window) {
		fprintf(stderr, "*** Failed to create window: %s\n", SDL_GetError());
		return 1;
	}

	//
	// get a pointer to keyboard state managed by SDL
	//
	keys = SDL_GetKeyboardState(NULL);

	//
	// create a renderer that takes care of drawing stuff to the window
	//
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		fprintf(stderr, "*** Failed to create renderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		return 1;
	}

	//
	// initialize colors
	//
	white.r = 255;
	white.g = 255;
	white.b = 255;

	red.r = 255;
	red.g = 0;
	red.b = 0;

	green.r = 0;
	green.g = 255;
	green.b = 0;

	//
	// initialize the sprite rectangles
	//

	const int PADDLE_W = 8;
	const int PADDLE_H = 64;

	// player 1 paddle
	p1.w = PADDLE_W;
	p1.h = PADDLE_H;
	p1.x = p1.w; // paddle is away from the wall
	p1.y = (scrHeight - p1.h)/2;

	// player 2 paddle
	p2.w = PADDLE_W;
	p2.h = PADDLE_H;
	p2.x = scrWidth - (p2.w * 2); // paddle is away from the wall
	p2.y = (scrHeight - p2.h) / 2;

	// ball
	ball.w = 8;
	ball.h = 8;
	ball.x = (scrWidth - ball.w) / 2;
	ball.y = (scrHeight - ball.h) / 2;

	//
	// enter the main loop where we process events, update the world, and draw everything
	//

	printf("Press F1 for controls.\n[1] Classic vs. AI\n[2] Classic vs. Human\n[3] RWG Mode vs. AI\n[4] RWG Mode vs. Human\n\n");
	while (!done) {
		//
		// handle events
		//
		SDL_Event e;    // structure that receives event information from SDL

		while (SDL_PollEvent(&e)) {

			switch (e.type) {
			case SDL_QUIT:
				printf("User closed the window");
				done = 1;
				break;

			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
					if (gameOn == 1)
					{
						gameOn = 0;
						ballInPlay = 0;
						sprintf(title, "%d-%d", 0, 0);
						SDL_SetWindowTitle(window, title);
						printf("Press F1 for controls.\n[1] Classic vs. AI\n[2] Classic vs. Human\n[3] RWG Mode vs. AI\n[4] RWG Mode vs. Human\n\n");
					}
					else
					{
						done = 1;  // set quit flag
					}
					break;
				case SDLK_1:
					if (gameOn == 0)
					{
						multiplayer = 0;
						gameOn = 1;
						RWGMode = 0;

						// reset score
						p1Score = 0;
						p2Score = 0;

						sprintf(title, "%d-%d", p1Score, p2Score);
						SDL_SetWindowTitle(window, title);

						// reset paddle positions
						p1.x = p1.w; // paddle is away from the wall
						p1.y = (scrHeight - p1.h) / 2;

						p2.x = scrWidth - (p2.w * 2); // paddle is away from the wall
						p2.y = (scrHeight - p2.h) / 2;

						// reset ball
						ball.x = (scrWidth - ball.w) / 2;
						ball.y = (scrHeight - ball.h) / 2;
						ballSpeedX = 0;
						ballSpeedY = 0;

						// reset colors
						p1ColorSetting = 0;
						p2ColorSetting = 0;
						ballColorSetting = 0;
					}
					break;
				case SDLK_2:
					if (gameOn == 0)
					{
						multiplayer = 1;
						gameOn = 1;
						RWGMode = 0;

						// reset score
						p1Score = 0;
						p2Score = 0;

						sprintf(title, "%d-%d", p1Score, p2Score);
						SDL_SetWindowTitle(window, title);

						// reset paddle positions
						p1.x = p1.w; // paddle is away from the wall
						p1.y = (scrHeight - p1.h) / 2;

						p2.x = scrWidth - (p2.w * 2); // paddle is away from the wall
						p2.y = (scrHeight - p2.h) / 2;

						// reset ball
						ball.x = (scrWidth - ball.w) / 2;
						ball.y = (scrHeight - ball.h) / 2;
						ballSpeedX = 0;
						ballSpeedY = 0;

						// reset colors
						p1ColorSetting = 0;
						p2ColorSetting = 0;
						ballColorSetting = 0;
					}
					break;
				case SDLK_3:
					if (gameOn == 0)
					{
						multiplayer = 0;
						gameOn = 1;
						RWGMode = 1;

						// reset score
						p1Score = 0;
						p2Score = 0;

						sprintf(title, "%d-%d", p1Score, p2Score);
						SDL_SetWindowTitle(window, title);

						// reset paddle positions
						p1.x = p1.w; // paddle is away from the wall
						p1.y = (scrHeight - p1.h) / 2;

						p2.x = scrWidth - (p2.w * 2); // paddle is away from the wall
						p2.y = (scrHeight - p2.h) / 2;

						// reset ball
						ball.x = (scrWidth - ball.w) / 2;
						ball.y = (scrHeight - ball.h) / 2;
						ballSpeedX = 0;
						ballSpeedY = 0;

						// reset colors
						p1ColorSetting = 0;
						p2ColorSetting = 0;
						ballColorSetting = 0;
					}
					break;
				case SDLK_4:
					if (gameOn == 0)
					{
						multiplayer = 1;
						gameOn = 1;
						RWGMode = 1;

						// reset score
						p1Score = 0;
						p2Score = 0;

						sprintf(title, "%d-%d", p1Score, p2Score);
						SDL_SetWindowTitle(window, title);

						// reset paddle positions
						p1.x = p1.w; // paddle is away from the wall
						p1.y = (scrHeight - p1.h) / 2;

						p2.x = scrWidth - (p2.w * 2); // paddle is away from the wall
						p2.y = (scrHeight - p2.h) / 2;

						// reset ball
						ball.x = (scrWidth - ball.w) / 2;
						ball.y = (scrHeight - ball.h) / 2;
						ballSpeedX = 0;
						ballSpeedY = 0;

						// reset colors
						p1ColorSetting = 0;
						p2ColorSetting = 0;
						ballColorSetting = 0;
					}
					break;
				case SDLK_SPACE:
					if (ballInPlay == 0 && gameOn == 1)
					{
						ballInPlay = 1;

						// randomize initial speed and direction
						ballSpeedX = 2;
						ballSpeedY = rand() % 3;

						if (rand() % 2 == 0) // 50/50 to start moving up/down
						{
							ballSpeedY *= -1;
						}

						if (lastPoint == 0) // new game
						{
							if (rand() % 2 == 0) // 50/50 to move toward P1
							{
								ballDirX = -1;
							}
						}
						else if (lastPoint == 1) // if P1 scored last, move towards P2
						{
							ballDirX = 1;
						}
						else if (lastPoint == 2) // if P2 scored last, move towards P1
						{
							ballDirX = -1;
						}

					}
					break;
				}
				break;
			}
		}

		//
		// function controls
		//

		// help 
		if (keys[SDL_SCANCODE_F1] && helpLock == 0)
		{
			helpLock = 1;
			printf("CONTROLS:\nPlayer 1 uses W/S to move Up/Down\nPlayer 2 uses UP/DOWN arrows to move Up/Down\nSPACE = Serve Ball\nESC = Back to Main Menu / Quit\n\nRWG Additional Controls:\nPlayer 1 uses A/D to switch colours.\nPlayer 2 uses LEFT/RIGHT arrows to switch colours.\nYour paddle must match the ball's colour to hit it.\n\nF1 = Show Controls (This screen)\nF2 = Toggle Scanlines\n\n");
		}
		else if (!keys[SDL_SCANCODE_F1] && helpLock == 1)
		{
			helpLock = 0;
		}

		// scanlines
		if (keys[SDL_SCANCODE_F2] && scanlinesLock == 0)
		{
			scanlinesLock = 1;
			scanlines *= -1;
		}
		else if (!keys[SDL_SCANCODE_F2] && scanlinesLock == 1)
		{
			scanlinesLock = 0;
		}		

		if (gameOn == 1)
		{
			//
			// update paddle position based on current keyboard state
			//

			// player 1 controls
			if (keys[SDL_SCANCODE_W])
			{
				p1.y -= paddleSpeed;
			}
			if (keys[SDL_SCANCODE_S])
			{
				p1.y += paddleSpeed;
			}

			// player 2 controls
			if (multiplayer == 1)
			{
				if (keys[SDL_SCANCODE_UP])
				{
					p2.y -= paddleSpeed;
				}
				if (keys[SDL_SCANCODE_DOWN])
				{
					p2.y += paddleSpeed;
				}
			}
			else // AI controls
			{
				int aiDetectRange = 3; // min of 2, larger numbers make ai detect ball farther away

				if (ballCenter.x > scrWidth / aiDetectRange && ballDirX == 1) // if ball on AI side and headed towards AI
				{
					if (ballCenter.y > p2Center.y + paddleSpeed) // move down to ball
					{
						p2.y += paddleSpeed;
						aiMovement = 1;
					}
					else if (ballCenter.y < p2Center.y - paddleSpeed) // move up to ball
					{
						p2.y -= paddleSpeed;
						aiMovement = -1;
					}
				}
				else
				{
					aiMovement = 0;
				}
			}

			// RWG Controls
			if (RWGMode == 1)
			{
				// player 1
				if (keys[SDL_SCANCODE_A] && p1AColorSwitchLock == 0)
				{
					p1AColorSwitchLock = 1;
					p1ColorSetting--;
					if (p1ColorSetting < 0)
					{
						p1ColorSetting = 2;
					}
				}
				else if (!keys[SDL_SCANCODE_A] && p1AColorSwitchLock == 1)
				{
					p1AColorSwitchLock = 0;
				}
				if (keys[SDL_SCANCODE_D] && p1DColorSwitchLock == 0)
				{
					p1DColorSwitchLock = 1;
					p1ColorSetting++;
					if (p1ColorSetting > 2)
					{
						p1ColorSetting = 0;
					}
				}
				else if (!keys[SDL_SCANCODE_D] && p1DColorSwitchLock == 1)
				{
					p1DColorSwitchLock = 0;
				}

				// player 2
				if (multiplayer == 1)
				{
					if (keys[SDL_SCANCODE_LEFT] && p2LColorSwitchLock == 0)
					{
						p2LColorSwitchLock = 1;
						p2ColorSetting--;
						if (p2ColorSetting < 0)
						{
							p2ColorSetting = 2;
						}
					}
					else if (!keys[SDL_SCANCODE_LEFT] && p2LColorSwitchLock == 1)
					{
						p2LColorSwitchLock = 0;
					}
					if (keys[SDL_SCANCODE_RIGHT] && p2RColorSwitchLock == 0)
					{
						p2RColorSwitchLock = 1;
						p2ColorSetting++;
						if (p2ColorSetting > 2)
						{
							p2ColorSetting = 0;
						}
					}
					else if (!keys[SDL_SCANCODE_RIGHT] && p2RColorSwitchLock == 1)
					{
						p2RColorSwitchLock = 0;
					}
				}
				else // AI RWG Controls
				{
					if (ballDirX > 0 && ballCenter.x > scrWidth / 2)
					{
						p2ColorSetting = ballColorSetting; // always match ball color
					}
				}
			}
			

			// ball movement
			if (ballInPlay != 0)
			{
				ball.x += ballSpeedX * ballDirX;
				ball.y += ballSpeedY;
			}
			if (ballHits >= hitsPerSpeedUp)
			{
				ballHits = 0;
				if (ballSpeedX < ballSpeedCapX)
				{
					ballSpeedX++;
				}
			}


			// player 1 boundary collision
			if (p1.y < 0)
			{
				p1.y = 0;
			}
			if (p1.y > scrHeight - p1.h)
			{
				p1.y = scrHeight - p1.h;
			}

			// player 2 boundary collision
			if (p2.y < 0)
			{
				p2.y = 0;
			}
			if (p2.y > scrHeight - p2.h)
			{
				p2.y = scrHeight - p2.h;
			}

			// ball boundary collision
			if (ball.y < 0)
			{
				printf("COLLISION: Top Wall\n");
				ball.y = 0;
				ballSpeedY *= -1;
				/* TOO HARD
				if (RWGMode == 1)
				{
					ballColorSetting = rand() % 3;
				}
				*/
			}
			if (ball.y > scrHeight - ball.h)
			{
				printf("COLLISION: Bottom Wall\n");
				ball.y = scrHeight - ball.h;
				ballSpeedY *= -1;
				/* TOO HARD
				if (RWGMode == 1)
				{
					ballColorSetting = rand() % 3;
				}
				*/
			}

			// calculate center of rectangles

			p1Center.x = p1.x + (p1.w / 2);
			p1Center.y = p1.y + (p1.h / 2);

			p2Center.x = p2.x + (p2.w / 2);
			p2Center.y = p2.y + (p2.h / 2);

			ballCenter.x = ball.x + (ball.w / 2);
			ballCenter.y = ball.y + (ball.h / 2);

			//
			// ball and paddle collision
			//

			SDL_bool p1Collision = SDL_HasIntersection(&ball, &p1);
			SDL_bool p2Collision = SDL_HasIntersection(&ball, &p2);

			// classic physics: https://www.youtube.com/watch?v=SHsYjWm8XSI
			
			if (p1Collision && ballDirX == -1 && (p1ColorSetting == ballColorSetting))
			{
				printf("COLLISION: Player 1\n");
				ballDirX *= -1;
				ballHits++;

				if (keys[SDL_SCANCODE_W] && ballSpeedY > -ballSpeedCapY) // paddle moving up
				{
					ballSpeedY--;
				}
				else if (keys[SDL_SCANCODE_S] && ballSpeedY < ballSpeedCapY) // paddle moving down
				{
					ballSpeedY++;
				}

				if (RWGMode == 1)
				{
					ballColorSetting = rand() % 3;
				}
			}
			else if (p2Collision && ballDirX == 1 && (p2ColorSetting == ballColorSetting))
			{
				printf("COLLISION: Player 2\n");
				ballDirX *= -1;
				ballHits++;

				if ((keys[SDL_SCANCODE_UP] && ballSpeedY > -ballSpeedCapY) || (multiplayer == 1 && aiMovement == -1))
				{
					ballSpeedY--;
				}
				else if ((keys[SDL_SCANCODE_DOWN] && ballSpeedY < ballSpeedCapY) || (multiplayer == 1 && aiMovement == 1))
				{
					ballSpeedY++;
				}

				if (RWGMode == 1)
				{
					ballColorSetting = rand() % 3;
				}
			}

			/*
			//FANCY PHYSICS

			if (p1Collision || p2Collision) // collision with either paddle
			{
				printf("COLLISION: ");

				// collision math: http://gamedev.stackexchange.com/questions/24078/which-side-was-hit/24091#24091

				// player 1 collision
				if (p1Collision)
				{
					int wy = (ball.w + p1.w) * (ballCenter.y - p1Center.y);
					int hx = (ball.h + p1.h) * (ballCenter.x - p1Center.x);

					if (wy > hx)
					{
						if (wy > -hx)
						{
							printf("Player 1 - BOTTOM\n");

							ball.y = p1.y + p1.h;
							ballSpeedY *= -1;
						}
						else
						{
							printf("Player 1 - LEFT\n");

							ball.x = p1.x - ball.w;
							ballSpeedX *= -1;
						}
					}
					else
					{
						if (wy > -hx)
						{
							printf("Player 1 - RIGHT\n");

							ball.x = p1.x + p1.w;
							ballSpeedX *= -1;
						}
						else
						{
							printf("Player 1 - TOP\n");

							ball.y = p1.y - ball.h;
							ballSpeedY *= -1;
						}
					}
					//printf("wy: %d\nhx: %d\n", wy, hx);
				}

				// player 2 collision
				if (p2Collision)
				{
					double wy = (double)(ball.w + p2.w) * (ballCenter.y - p2Center.y);
					double hx = (double)(ball.h + p2.h) * (ballCenter.x - p2Center.x);

					if (wy > hx)
					{
						if (wy > -hx)
						{
							printf("Player 2 - BOTTOM\n");
							ball.y = p2.y + p2.h;
							ballSpeedY *= -1;
						}
						else
						{
							printf("Player 2 - LEFT\n");

							ball.x = p2.x - ball.w;
							ballSpeedX *= -1;
						}
					}
					else
					{
						if (wy > -hx)
						{
							printf("Player 2 - RIGHT\n");

							ball.x = p2.x + p2.w;
							ballSpeedX *= -1;
						}
						else
						{
							printf("Player 2 - TOP\n");

							ball.y = p2.y - ball.h;
							ballSpeedY *= -1;
						}
					}
				}
				//printf("wy: %d\nhx: %d\n", wy, hx);

			}*/

			// ball out of bounds
			if (ball.x < 0 - ball.w || ball.x > scrWidth) // score
			{
				if (ball.x > scrWidth)	// player 1 score
				{
					p1Score++;
					lastPoint = 1;
				}
				else					// player 2 score
				{
					p2Score++;
					lastPoint = 2;
				}

				// play is paused
				ballInPlay = 0;

				// reset colours
				p1ColorSetting = 0;
				p2ColorSetting = 0;
				ballColorSetting = 0;
				
				// reset ball position
				ball.x = (scrWidth - ball.w) / 2;
				ball.y = (scrHeight - ball.h) / 2;

				// stop ball at reset location
				ballSpeedX = 0;
				ballSpeedY = 0;

				// change title
				sprintf(title, "%d-%d", p1Score, p2Score);
				SDL_SetWindowTitle(window, title);

				// print score
				printf("SCORE: %d-%d\n", p1Score, p2Score);
				printf("Last Point: %d\n", lastPoint);

				// win score reached
				if (p1Score >= winScore || p2Score >= winScore)
				{
					gameOn = 0;
					if (p1Score >= winScore)
					{
						printf("Player 1 wins!\n");
					}
					else if (multiplayer == 1)
					{
						printf("Player 2 wins!\n");
					}
					else
					{
						printf("AI wins!\n");
					}
					printf("Press F1 for controls.\n[1] Classic vs. AI\n[2] Classic vs. Human\n[3] RWG Mode vs. AI\n[4] RWG Mode vs. Human\n\n");
				}
			}


			// player 1 color
			switch (p1ColorSetting) {
			case 0:
				p1Color = &white;
				break;
			case 1:
				p1Color = &red;
				break;
			case 2:
				p1Color = &green;
				break;
			}

			// player 2 color
			switch (p2ColorSetting) {
			case 0:
				p2Color = &white;
				break;
			case 1:
				p2Color = &red;
				break;
			case 2:
				p2Color = &green;
				break;
			}

			// ball color
			switch (ballColorSetting) {
			case 0:
				ballColor = &white;
				break;
			case 1:
				ballColor = &red;
				break;
			case 2:
				ballColor = &green;
				break;
			}
		}

		//
		// draw everything
		//

		// background
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// half line
		SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		SDL_RenderDrawLine(renderer, scrWidth / 2, 0, scrWidth / 2, scrHeight);

		if (gameOn == 1)
		{
			// p1 paddle
			SDL_SetRenderDrawColor(renderer, p1Color->r, p1Color->g, p1Color->b, 255);
			SDL_RenderFillRect(renderer, &p1);

			// p2 paddle
			SDL_SetRenderDrawColor(renderer, p2Color->r, p2Color->g, p2Color->b, 255);
			SDL_RenderFillRect(renderer, &p2);

			// ball
			if (ballInPlay != 0)
			{
				SDL_SetRenderDrawColor(renderer, ballColor->r, ballColor->g, ballColor->b, 255);
				SDL_RenderFillRect(renderer, &ball);
			}
		}
		//
		// score
		//
		SDL_Rect hr; // horizontal number piece, default position
		hr.w = 16;
		hr.h = 4;

		SDL_Rect vr; // vertical number piece, default position
		vr.w = 4;
		vr.h = 16;

		int scoreDisplayWidth = hr.w + vr.w + hr.w;

		// score display offset from (0,0), or ((scrWidth - scoreDisplayWidth), 0)
		int scoreDisplayOffsetX = (scrWidth / 4) - (scoreDisplayWidth / 2);
		int scoreDisplayOffsetY = 4;

		int scoreOnesDigitOffsetX = hr.w + vr.w; // ones digit offset from tens digit

		if (p1Score < winScore)
		{
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		}
		else
		{
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		}

		// p1 score, ones
		if (scoreDisplay[p1Score % 10][0] == 1)
		{			
			hr.x = 0 + scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			hr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p1Score % 10][1] == 1)
		{
			vr.x = 0 + scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score % 10][2] == 1)
		{
			vr.x = 12 + scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score % 10][3] == 1)
		{
			hr.x = 0 + scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			hr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p1Score % 10][4] == 1)
		{
			vr.x = 0 + scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score % 10][5] == 1)
		{
			vr.x = 12 + scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score % 10][6] == 1)
		{
			hr.x = 0 + scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			hr.y = 24 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}

		// p1 score, tens
		if (scoreDisplay[p1Score / 10][0] == 1)
		{
			hr.x = 0 + scoreDisplayOffsetX;
			hr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p1Score / 10][1] == 1)
		{
			vr.x = 0 + scoreDisplayOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score / 10][2] == 1)
		{
			vr.x = 12 + scoreDisplayOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score / 10][3] == 1)
		{
			hr.x = 0 + scoreDisplayOffsetX;
			hr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p1Score / 10][4] == 1)
		{
			vr.x = 0 + scoreDisplayOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score / 10][5] == 1)
		{
			vr.x = 12 + scoreDisplayOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p1Score / 10][6] == 1)
		{
			hr.x = 0 + scoreDisplayOffsetX;
			hr.y = 24 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}

		if (p2Score < winScore)
		{
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		}
		else
		{
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		}

		// p2 score, ones
		if (scoreDisplay[p2Score % 10][0] == 1)
		{
			hr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			hr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p2Score % 10][1] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score % 10][2] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) + 12 - scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score % 10][3] == 1)
		{
			hr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			hr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p2Score % 10][4] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score % 10][5] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) + 12 - scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score % 10][6] == 1)
		{
			hr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX + scoreOnesDigitOffsetX;
			hr.y = 24 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}

		// p2 score, tens
		if (scoreDisplay[p2Score / 10][0] == 1)
		{
			hr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX;
			hr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p2Score / 10][1] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score / 10][2] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) + 12 - scoreDisplayOffsetX;
			vr.y = 0 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score / 10][3] == 1)
		{
			hr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX;
			hr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}
		if (scoreDisplay[p2Score / 10][4] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score / 10][5] == 1)
		{
			vr.x = (scrWidth - scoreDisplayWidth) + 12 - scoreDisplayOffsetX;
			vr.y = 12 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &vr);
		}
		if (scoreDisplay[p2Score / 10][6] == 1)
		{
			hr.x = (scrWidth - scoreDisplayWidth) - scoreDisplayOffsetX;
			hr.y = 24 + scoreDisplayOffsetY;

			SDL_RenderFillRect(renderer, &hr);
		}

		// scanlines
		if (scanlines == 1)
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			for (int y = 1; y < scrHeight; y += 2)
			{
				SDL_RenderDrawLine(renderer, 0, y, scrWidth, y);
			}
		}		

		// display everything we just drew
		SDL_RenderPresent(renderer);
	}

	// this closes the window and shuts down SDL
	SDL_Quit();

	// we're done
	return 0;
}
