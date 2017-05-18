#include "components/VideoVlcComponent.h"
#include "Renderer.h"
#include "ThemeData.h"
#include "Util.h"
#ifdef WIN32
#include <codecvt>
#endif

libvlc_instance_t*		VideoVlcComponent::mVLC = NULL;

// VLC prepares to render a video frame.
static void *lock(void *data, void **p_pixels) {
    struct VideoContext *c = (struct VideoContext *)data;
    SDL_LockMutex(c->mutex);
    SDL_LockSurface(c->surface);
	*p_pixels = c->surface->pixels;
    return NULL; // Picture identifier, not needed here.
}

// VLC just rendered a video frame.
static void unlock(void *data, void *id, void *const *p_pixels) {
    struct VideoContext *c = (struct VideoContext *)data;
    SDL_UnlockSurface(c->surface);
    SDL_UnlockMutex(c->mutex);
}

// VLC wants to display a video frame.
static void display(void *data, void *id) {
    //Data to be displayed
}

VideoVlcComponent::VideoVlcComponent(Window* window) :
	VideoComponent(window),
	mMediaPlayer(nullptr)
{
	memset(&mContext, 0, sizeof(mContext));

	// Get an empty texture for rendering the video
	mTexture = TextureResource::get("");

	// Make sure VLC has been initialised
	setupVLC();
}

VideoVlcComponent::~VideoVlcComponent()
{
}

void VideoVlcComponent::render(const Eigen::Affine3f& parentTrans)
{
	VideoComponent::render(parentTrans);
	float x, y;

	Eigen::Affine3f trans = parentTrans * getTransform();
	GuiComponent::renderChildren(trans);

	Renderer::setMatrix(trans);
	
	if (mIsPlaying && mContext.valid)
	{
		float tex_offs_x = 0.0f;
		float tex_offs_y = 0.0f;
		float x2;
		float y2;

		x = -(float)mSize.x() * mOrigin.x();
		y = -(float)mSize.y() * mOrigin.y();
		x2 = x+mSize.x();
		y2 = y+mSize.y();

		// Define a structure to contain the data for each vertex
		struct Vertex
		{
			Eigen::Vector2f pos;
			Eigen::Vector2f tex;
			Eigen::Vector4f colour;
		} vertices[6];

		// We need two triangles to cover the rectangular area
		vertices[0].pos[0] = x; 			vertices[0].pos[1] = y;
		vertices[1].pos[0] = x; 			vertices[1].pos[1] = y2;
		vertices[2].pos[0] = x2;			vertices[2].pos[1] = y;

		vertices[3].pos[0] = x2;			vertices[3].pos[1] = y;
		vertices[4].pos[0] = x; 			vertices[4].pos[1] = y2;
		vertices[5].pos[0] = x2;			vertices[5].pos[1] = y2;

		// Texture coordinates
		vertices[0].tex[0] = -tex_offs_x; 			vertices[0].tex[1] = -tex_offs_y;
		vertices[1].tex[0] = -tex_offs_x; 			vertices[1].tex[1] = 1.0f + tex_offs_y;
		vertices[2].tex[0] = 1.0f + tex_offs_x;		vertices[2].tex[1] = -tex_offs_y;

		vertices[3].tex[0] = 1.0f + tex_offs_x;		vertices[3].tex[1] = -tex_offs_y;
		vertices[4].tex[0] = -tex_offs_x;			vertices[4].tex[1] = 1.0f + tex_offs_y;
		vertices[5].tex[0] = 1.0f + tex_offs_x;		vertices[5].tex[1] = 1.0f + tex_offs_y;

		// Colours - use this to fade the video in and out
		for (int i = 0; i < (4 * 6); ++i) {
			if ((i%4) < 3)
				vertices[i / 4].colour[i % 4] = mFadeIn;
			else
				vertices[i / 4].colour[i % 4] = 1.0f;
		}

		glEnable(GL_TEXTURE_2D);

		// Build a texture for the video frame
		mTexture->initFromPixels((unsigned char*)mContext.surface->pixels, mContext.surface->w, mContext.surface->h);
		mTexture->bind();

		// Render it
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glColorPointer(4, GL_FLOAT, sizeof(Vertex), &vertices[0].colour);
		glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].pos);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].tex);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glDisable(GL_TEXTURE_2D);
	}
}

void VideoVlcComponent::setupContext()
{
	if (!mContext.valid)
	{
		// Create an RGBA surface to render the video into
		mContext.surface = SDL_CreateRGBSurface(SDL_SWSURFACE, (int)mVideoWidth, (int)mVideoHeight, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
		mContext.mutex = SDL_CreateMutex();
		mContext.valid = true;
	}
}

void VideoVlcComponent::freeContext()
{
	if (mContext.valid)
	{
		SDL_FreeSurface(mContext.surface);
		SDL_DestroyMutex(mContext.mutex);
		mContext.valid = false;
	}
}

void VideoVlcComponent::setupVLC()
{
	// If VLC hasn't been initialised yet then do it now
	if (!mVLC)
	{
		const char* args[] = { "--quiet" };
		mVLC = libvlc_new(sizeof(args) / sizeof(args[0]), args);
	}
}

void VideoVlcComponent::handleLooping()
{
	if (mIsPlaying && mMediaPlayer)
	{
		libvlc_state_t state = libvlc_media_player_get_state(mMediaPlayer);
		if (state == libvlc_Ended)
		{
			//libvlc_media_player_set_position(mMediaPlayer, 0.0f);
			libvlc_media_player_set_media(mMediaPlayer, mMedia);
			libvlc_media_player_play(mMediaPlayer);
		}
	}
}

void VideoVlcComponent::startVideo()
{
	if (!mIsPlaying) {
		mVideoWidth = 0;
		mVideoHeight = 0;

#ifdef WIN32
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wton;
		std::string path = wton.to_bytes(mVideoPath.c_str());
#else
		std::string path(mVideoPath.c_str());
#endif
		// Make sure we have a video path
		if (mVLC && (path.size() > 0))
		{
			// Set the video that we are going to be playing so we don't attempt to restart it
			mPlayingVideoPath = mVideoPath;

			// Open the media
			mMedia = libvlc_media_new_path(mVLC, path.c_str());
			if (mMedia)
			{
				unsigned 	track_count;
				// Get the media metadata so we can find the aspect ratio
				libvlc_media_parse(mMedia);
				libvlc_media_track_t** tracks;
				track_count = libvlc_media_tracks_get(mMedia, &tracks);
				for (unsigned track = 0; track < track_count; ++track)
				{
					if (tracks[track]->i_type == libvlc_track_video)
					{
						mVideoWidth = tracks[track]->video->i_width;
						mVideoHeight = tracks[track]->video->i_height;
						break;
					}
				}
				libvlc_media_tracks_release(tracks, track_count);

				// Make sure we found a valid video track
				if ((mVideoWidth > 0) && (mVideoHeight > 0))
				{
					setupContext();

					// Setup the media player
					mMediaPlayer = libvlc_media_player_new_from_media(mMedia);
					libvlc_media_player_play(mMediaPlayer);
					libvlc_video_set_callbacks(mMediaPlayer, lock, unlock, display, (void*)&mContext);
					libvlc_video_set_format(mMediaPlayer, "RGBA", (int)mVideoWidth, (int)mVideoHeight, (int)mVideoWidth * 4);

					// Update the playing state
					mIsPlaying = true;
					mFadeIn = 0.0f;
				}
			}
		}
	}
}

void VideoVlcComponent::stopVideo()
{
	mIsPlaying = false;
	mStartDelayed = false;
	// Release the media player so it stops calling back to us
	if (mMediaPlayer)
	{
		libvlc_media_player_stop(mMediaPlayer);
		libvlc_media_player_release(mMediaPlayer);
		libvlc_media_release(mMedia);
		mMediaPlayer = NULL;
		freeContext();
	}
}

