#pragma once

class Scene
{
public:

	/// <summary>
	/// ‰Šú‰»
	/// </summary>
	/// <returns>‰Šú‰»¬Œ÷‚É•Ô‚·</returns>
	bool Init();

	/// <summary>
	/// XVˆ—
	/// </summary>
	void Update();

	/// <summary>
	/// •`‰æˆ—
	/// </summary>
	void Draw();

private:

};

extern Scene* sceneInstance;