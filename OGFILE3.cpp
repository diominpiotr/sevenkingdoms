/*
 * Seven Kingdoms: Ancient Adversaries
 *
 * Copyright 1997,1998 Enlight Software Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//Filename    : OGFILE3.CPP
//Description : Object Game file, save game and restore game, part 3

#include <OUNIT.h>

#include <OBULLET.h>
#include <OU_MARI.h>
#include <OB_PROJ.h>
#include <OSITE.h>
#include <OTOWN.h>
#include <ONATION.h>
#include <OFIRM.h>
#include <OTORNADO.h>
#include <OREBEL.h>
#include <OSPY.h>
#include <OSNOWG.h>
#include <OREGION.h>
#include <OREGIONS.h>
#include <ONEWS.h>
#include <OGFILE.h>

#include <OGF_V1.h>

//------- declare static functions -------//

static char* create_monster_func();
static char* create_rebel_func();

static void write_ai_info(File* filePtr, short* aiInfoArray, short aiInfoCount, short aiInfoSize);
static void read_ai_info(File* filePtr, short** aiInfoArrayPtr, short& aiInfoCount, short& aiInfoSize);


//-------- Start of function UnitArray::write_file -------------//
//
int UnitArray::write_file(File* filePtr)
{
   int  i;
   Unit *unitPtr;

	filePtr->file_put_short(restart_recno);  // variable in SpriteArray

	filePtr->file_put_short( size()  );  // no. of units in unit_array

	filePtr->file_put_short( selected_recno );
	filePtr->file_put_short( selected_count );
	filePtr->file_put_long ( cur_group_id   );
	filePtr->file_put_long ( cur_team_id    );
	filePtr->file_put_short(idle_blocked_unit_reset_count);
	filePtr->file_put_long (unit_search_tries);
	filePtr->file_put_short(unit_search_tries_flag);

	filePtr->file_put_short(visible_unit_count);
	filePtr->file_put_short(mp_first_frame_to_select_caravan);
	filePtr->file_put_short(mp_first_frame_to_select_ship);
	filePtr->file_put_short(mp_pre_selected_caravan_recno);
	filePtr->file_put_short(mp_pre_selected_ship_recno);

	for( i=1; i<=size() ; i++ )
   {
      unitPtr = (Unit*) get_ptr(i);

      //----- write unitId or 0 if the unit is deleted -----//

      if( !unitPtr )    // the unit is deleted
      {
         filePtr->file_put_short(0);
      }
      else
      {
         //--------- write unit_id -------------//

         filePtr->file_put_short(unitPtr->unit_id);

         //------ write data in the base class ------//

         if( !unitPtr->write_file(filePtr) )
            return 0;

         //------ write data in the derived class ------//

         if( !unitPtr->write_derived_file(filePtr) )
				return 0;
      }
   }

   //------- write empty room array --------//

   write_empty_room(filePtr);

   return 1;
}
//--------- End of function UnitArray::write_file ---------------//


//-------- Start of function UnitArray::read_file -------------//
//
int UnitArray::read_file(File* filePtr)
{
	Unit*   unitPtr;
	int     i, unitId, emptyRoomCount=0;

	restart_recno    = filePtr->file_get_short();

	int unitCount    = filePtr->file_get_short();  // get no. of units from file

	selected_recno   = filePtr->file_get_short();
	selected_count   = filePtr->file_get_short();
	cur_group_id     = filePtr->file_get_long();
	cur_team_id      = filePtr->file_get_long();
	idle_blocked_unit_reset_count = filePtr->file_get_short();
	unit_search_tries	= filePtr->file_get_long ();
	unit_search_tries_flag = (char) filePtr->file_get_short();

   visible_unit_count					= filePtr->file_get_short();
	mp_first_frame_to_select_caravan = (char) filePtr->file_get_short();
	mp_first_frame_to_select_ship		= (char) filePtr->file_get_short();
	mp_pre_selected_caravan_recno		= filePtr->file_get_short();
	mp_pre_selected_ship_recno			= filePtr->file_get_short();

   for( i=1 ; i<=unitCount ; i++ )
   {
      unitId = filePtr->file_get_short();

      if( unitId==0 )  // the unit has been deleted
      {
         add_blank(1);     // it's a DynArrayB function
         emptyRoomCount++;
      }
      else
      {
         //----- create unit object -----------//

			unitPtr = create_unit( unitId );
         unitPtr->unit_id = unitId;

         //---- read data in base class -----//

         if( !unitPtr->read_file( filePtr ) )
            return 0;

         //----- read data in derived class -----//

         if( !unitPtr->read_derived_file( filePtr ) )
            return 0;
      }
   }

	//-------- linkout() those record added by add_blank() ----------//
   //-- So they will be marked deleted in DynArrayB and can be -----//
	//-- undeleted and used when a new record is going to be added --//

   for( i=size() ; i>0 ; i-- )
	{
		DynArrayB::go(i);             // since UnitArray has its own go() which will call GroupArray::go()

      if( get_ptr() == NULL )       // add_blank() record
         linkout();
   }

   //------- read empty room array --------//

   read_empty_room(filePtr);

   //------- verify the empty_room_array loading -----//

#ifdef DEBUG
   err_when( empty_room_count != emptyRoomCount );

   for( i=0 ; i<empty_room_count ; i++ )
   {
      if( !is_deleted( empty_room_array[i].recno ) )
         err_here();
   }
#endif

   return 1;
}
//--------- End of function UnitArray::read_file ---------------//


//--------- Begin of function Unit::write_file ---------//
//
// Write data in derived class.
//
// If the derived Unit don't have any special data,
// just use Unit::write_file(), otherwise make its own derived copy of write_file()
//
int Unit::write_file(File* filePtr)
{
   if( !filePtr->file_write( this, sizeof(Unit) ) )
		return 0;

   //--------------- write memory data ----------------//

	if( result_node_array )
	{
		if( !filePtr->file_write( result_node_array, sizeof(ResultNode) * result_node_count ) )
			return 0;
	}

	//### begin alex 15/10 ###//
	if(way_point_array)
	{
		err_when(way_point_array_size==0 || way_point_array_size<way_point_count);
		if(!filePtr->file_write(way_point_array, sizeof(ResultNode)*way_point_array_size))
			return 0;
	}
	//#### end alex 15/10 ####//

	if( team_info )
	{
		if( !filePtr->file_write( team_info, sizeof(TeamInfo) ) )
			return 0;
   }

   return 1;
}
//----------- End of function Unit::write_file ---------//


//--------- Begin of function Unit::read_file ---------//
//
int Unit::read_file(File* filePtr)
{
   char* vfPtr = *((char**)this);      // save the virtual function table pointer

   if( !filePtr->file_read( this, sizeof(Unit) ) )
      return 0;

   *((char**)this) = vfPtr;

	//--------------- read in memory data ----------------//

	if( result_node_array )
	{
		result_node_array = (ResultNode*) mem_add( sizeof(ResultNode) * result_node_count );

		if( !filePtr->file_read( result_node_array, sizeof(ResultNode) * result_node_count ) )
			return 0;
	}

	//### begin alex 15/10 ###//
	if(way_point_array)
	{
		way_point_array = (ResultNode*) mem_add(sizeof(ResultNode) * way_point_array_size);

		if(!filePtr->file_read(way_point_array, sizeof(ResultNode)*way_point_array_size))
			return 0;
	}
	//#### end alex 15/10 ####//

	if( team_info )
	{
		team_info = (TeamInfo*) mem_add( sizeof(TeamInfo) );

		if( !filePtr->file_read( team_info, sizeof(TeamInfo) ) )
			return 0;
	}

	//----------- post-process the data read ----------//

	// attack_info_array = unit_res.attack_info_array+unit_res[unit_id]->first_attack-1;
	sprite_info       = sprite_res[sprite_id];

	sprite_info->load_bitmap_res();

	//--------- special process of UNIT_MARINE --------//

	// move to read_derived_file
	//if( unit_res[unit_id]->unit_class == UNIT_CLASS_SHIP )
	//{
	//	((UnitMarine*)this)->splash.sprite_info = sprite_res[sprite_id];
	//	((UnitMarine*)this)->splash.sprite_info->load_bitmap_res();
	//}

   return 1;
}
//----------- End of function Unit::read_file ---------//


//--------- Begin of function Unit::write_derived_file ---------//
//
int Unit::write_derived_file(File* filePtr)
{
   //--- write data in derived class -----//

	int writeSize = unit_array.unit_class_size(unit_id)-sizeof(Unit);

   if( writeSize > 0 )
   {
      if( !filePtr->file_write( (char*) this + sizeof(Unit), writeSize ) )
         return 0;
   }

   return 1;
}
//----------- End of function Unit::write_derived_file ---------//


//--------- Begin of function Unit::read_derived_file ---------//
//
int Unit::read_derived_file(File* filePtr)
{
	//--- read data in derived class -----//

   int readSize = unit_array.unit_class_size(unit_id) - sizeof(Unit);

   if( readSize > 0 )
   {
      if( !filePtr->file_read( (char*) this + sizeof(Unit), readSize ) )
         return 0;
	}

	// ###### begin Gilbert 13/8 #######//
	fix_attack_info();
	// ###### end Gilbert 13/8 #######//

   return 1;
}
//----------- End of function Unit::read_derived_file ---------//

//--------- Begin of function UnitMarine::read_derived_file ---------//
int UnitMarine::read_derived_file(File* filePtr)
{
	//---- backup virtual functions table pointer of splash ----//
	char* splashVfPtr = *((char **)&splash);

	//--------- read file --------//
	if( !Unit::read_derived_file(filePtr) )
		return 0;

	// -------- restore virtual function table pointer -------//
	*((char **)&splash) = splashVfPtr ;

	// ------- post-process the data read --------//
	splash.sprite_info = sprite_res[splash.sprite_id];
	splash.sprite_info->load_bitmap_res();

	return 1;
}
//--------- End of function UnitMarine::read_derived_file ---------//


//*****//


//-------- Start of function BulletArray::write_file -------------//
//
int BulletArray::write_file(File* filePtr)
{
	filePtr->file_put_short(restart_recno);  // variable in SpriteArray

	int    i, emptyRoomCount=0;;
	Bullet *bulletPtr;

	filePtr->file_put_short( size() );  // no. of bullets in bullet_array

	for( i=1; i<=size() ; i++ )
	{
		bulletPtr = (Bullet*) get_ptr(i);

		//----- write bulletId or 0 if the bullet is deleted -----//

		if( !bulletPtr )    // the bullet is deleted
		{
			filePtr->file_put_short(0);
			emptyRoomCount++;
		}
		else
		{
			filePtr->file_put_short(bulletPtr->sprite_id);      // there is a bullet in this record

			//------ write data in the base class ------//

			if( !bulletPtr->write_file(filePtr) )
				return 0;

			//------ write data in the derived class -------//

			if( !bulletPtr->write_derived_file(filePtr) )
				return 0;
		}
	}

	//------- write empty room array --------//

	write_empty_room(filePtr);

	//------- verify the empty_room_array loading -----//

#ifdef DEBUG
	err_when( empty_room_count != emptyRoomCount );

   for( i=0 ; i<empty_room_count ; i++ )
   {
		if( !is_deleted( empty_room_array[i].recno ) )
         err_here();
   }
#endif

	return 1;
}
//--------- End of function BulletArray::write_file -------------//


//-------- Start of function BulletArray::read_file -------------//
//
int BulletArray::read_file(File* filePtr)
{
	restart_recno    = filePtr->file_get_short();

	int     i, bulletRecno, bulletCount, emptyRoomCount=0, spriteId;
	Bullet* bulletPtr;

	bulletCount = filePtr->file_get_short();  // get no. of bullets from file

	for( i=1 ; i<=bulletCount ; i++ )
	{
		spriteId = filePtr->file_get_short();
		if( spriteId == 0 )
		{
			add_blank(1);     // it's a DynArrayB function

			emptyRoomCount++;
		}
		else
		{
			//----- create bullet object -----------//

			bulletRecno = create_bullet(spriteId);
			bulletPtr   = bullet_array[bulletRecno];

         //----- read data in base class --------//

         if( !bulletPtr->read_file( filePtr ) )
            return 0;

			//----- read data in derived class -----//

			if( !bulletPtr->read_derived_file( filePtr ) )
				return 0;
      }
	}

   //-------- linkout() those record added by add_blank() ----------//
	//-- So they will be marked deleted in DynArrayB and can be -----//
	//-- undeleted and used when a new record is going to be added --//

	for( i=1 ; i<=size() ; i++ )
	{
		DynArrayB::go(i);             // since BulletArray has its own go() which will call GroupArray::go()

		if( get_ptr() == NULL )       // add_blank() record
			linkout();
	}

	//------- read empty room array --------//

	read_empty_room(filePtr);

	//------- verify the empty_room_array loading -----//

#ifdef DEBUG
	err_when( empty_room_count != emptyRoomCount );

	for( i=0 ; i<empty_room_count ; i++ )
	{
		if( !is_deleted( empty_room_array[i].recno ) )
			err_here();
	}
#endif

	return 1;
}
//--------- End of function BulletArray::read_file ---------------//


//--------- Begin of function Bullet::write_file ---------//
//
int Bullet::write_file(File* filePtr)
{
	if( !filePtr->file_write( this, sizeof(Bullet) ) )
		return 0;

	return 1;
}
//----------- End of function Bullet::write_file ---------//


//--------- Begin of function Bullet::read_file ---------//
//
int Bullet::read_file(File* filePtr)
{
	char* vfPtr = *((char**)this);      // save the virtual function table pointer

	if( !filePtr->file_read( this, sizeof(Bullet) ) )
		return 0;

	*((char**)this) = vfPtr;

   //------------ post-process the data read ----------//

	sprite_info = sprite_res[sprite_id];

	sprite_info->load_bitmap_res();

	return 1;
}
//----------- End of function Bullet::read_file ---------//


//----------- Begin of function Bullet::write_derived_file ---------//
int Bullet::write_derived_file(File *filePtr)
{
	//--- write data in derived class -----//

	int writeSize = bullet_array.bullet_class_size(sprite_id)-sizeof(Bullet);

	if( writeSize > 0 )
	{
		if( !filePtr->file_write( (char*) this + sizeof(Bullet), writeSize ) )
			return 0;
	}

	return 1;

}
//----------- End of function Bullet::write_derived_file ---------//


//----------- Begin of function Bullet::read_derived_file ---------//
int Bullet::read_derived_file(File *filePtr)
{
	//--- read data in derived class -----//

	int readSize = bullet_array.bullet_class_size(sprite_id) - sizeof(Bullet);

	if( readSize > 0 )
	{
		if( !filePtr->file_read( (char*) this + sizeof(Bullet), readSize ) )
			return 0;
	}

	return 1;
}
//----------- End of function Bullet::read_derived_file ---------//


//----------- Begin of function Projectile::read_derived_file ---------//

int Projectile::read_derived_file(File *filePtr)
{
	//--- backup virtual function table pointer of act_bullet and bullet_shadow ---//
   char* actBulletVfPtr = *((char**)&act_bullet);
   char* bulletShadowVfPtr = *((char**)&bullet_shadow);

	//---------- read file ----------//
	if( !Bullet::read_derived_file(filePtr) )
		return 0;

	//------ restore virtual function table pointer --------//
	*((char**)&act_bullet) = actBulletVfPtr;
	*((char**)&bullet_shadow) = bulletShadowVfPtr;

   //----------- post-process the data read ----------//
	act_bullet.sprite_info = sprite_res[act_bullet.sprite_id];
	act_bullet.sprite_info->load_bitmap_res();
	bullet_shadow.sprite_info = sprite_res[bullet_shadow.sprite_id];
	bullet_shadow.sprite_info->load_bitmap_res();

	return 1;
}
//----------- End of function Projectile::read_derived_file ---------//

//*****//

//-------- Start of function FirmArray::write_file -------------//
//
int FirmArray::write_file(File* filePtr)
{
   int  i;
   Firm *firmPtr;

   filePtr->file_put_short( size()  );  // no. of firms in firm_array
   filePtr->file_put_short( process_recno );
	filePtr->file_put_short( selected_recno );

	filePtr->file_put_short( Firm::firm_menu_mode );
	filePtr->file_put_short( Firm::action_spy_recno );
	filePtr->file_put_short( Firm::bribe_result );
	filePtr->file_put_short( Firm::assassinate_result );

	for( i=1; i<=size() ; i++ )
   {
      firmPtr = (Firm*) get_ptr(i);

      //----- write firmId or 0 if the firm is deleted -----//

      if( !firmPtr )    // the firm is deleted
		{
         filePtr->file_put_short(0);
      }
      else
      {
         //--------- write firm_id -------------//

         filePtr->file_put_short(firmPtr->firm_id);

         //------ write data in base class --------//

			if( !filePtr->file_write( firmPtr, sizeof(Firm) ) )
            return 0;

         //--------- write worker_array ---------//

         if( firmPtr->worker_array )
         {
            if( !filePtr->file_write( firmPtr->worker_array, MAX_WORKER*sizeof(Worker) ) )
               return 0;
         }

         //------ write data in derived class ------//

         if( !firmPtr->write_derived_file(filePtr) )
            return 0;
      }
   }

   //------- write empty room array --------//

	write_empty_room(filePtr);

   return 1;
}
//--------- End of function FirmArray::write_file ---------------//


//-------- Start of function FirmArray::read_file -------------//
//
int FirmArray::read_file(File* filePtr)
{
	Firm*   firmPtr;
	char*   vfPtr;
	int     i, firmId, firmRecno;

	int firmCount      = filePtr->file_get_short();  // get no. of firms from file
	process_recno      = filePtr->file_get_short();
	selected_recno     = filePtr->file_get_short();

	Firm::firm_menu_mode  	 = (char) filePtr->file_get_short();
	Firm::action_spy_recno   = filePtr->file_get_short();
	Firm::bribe_result    	 = (char) filePtr->file_get_short();
	Firm::assassinate_result = (char) filePtr->file_get_short();

   for( i=1 ; i<=firmCount ; i++ )
   {
      firmId = filePtr->file_get_short();

      if( firmId==0 )  // the firm has been deleted
      {
         add_blank(1);     // it's a DynArrayB function
      }
      else
      {
         //----- create firm object -----------//

         firmRecno = create_firm( firmId );
         firmPtr   = firm_array[firmRecno];

         //---- read data in base class -----//

         vfPtr = *((char**)firmPtr);      // save the virtual function table pointer

         if( !filePtr->file_read( firmPtr, sizeof(Firm) ) )
            return 0;


			#ifdef AMPLUS
				if(!game_file_array.same_version && firmPtr->firm_id > FIRM_BASE)
					firmPtr->firm_build_id += MAX_RACE - VERSION_1_MAX_RACE;
			#endif


         *((char**)firmPtr) = vfPtr;

         //--------- read worker_array ---------//

         if( firm_res[firmId]->need_worker )
         {
            firmPtr->worker_array = (Worker*) mem_add( MAX_WORKER*sizeof(Worker) );

            if( !filePtr->file_read( firmPtr->worker_array, MAX_WORKER*sizeof(Worker) ) )
               return 0;
         }

         //----- read data in derived class -----//

         if( !firmPtr->read_derived_file( filePtr ) )
            return 0;
      }
   }

   //-------- linkout() those record added by add_blank() ----------//
   //-- So they will be marked deleted in DynArrayB and can be -----//
   //-- undeleted and used when a new record is going to be added --//

   for( i=size() ; i>0 ; i-- )
   {
      DynArrayB::go(i);             // since FirmArray has its own go() which will call GroupArray::go()

      if( get_ptr() == NULL )       // add_blank() record
         linkout();
   }

   //------- read empty room array --------//

   read_empty_room(filePtr);

   return 1;
}
//--------- End of function FirmArray::read_file ---------------//


//--------- Begin of function Firm::write_derived_file ---------//
//
// Write data in derived class.
//
// If the derived Firm don't have any special data,
// just use Firm::write_file(), otherwise make its own derived copy of write_file()
//
int Firm::write_derived_file(File* filePtr)
{
   //--- write data in derived class -----//

   int writeSize = firm_array.firm_class_size(firm_id)-sizeof(Firm);

   if( writeSize > 0 )
   {
      if( !filePtr->file_write( (char*) this + sizeof(Firm), writeSize ) )
         return 0;
   }

   return 1;
}
//----------- End of function Firm::write_derived_file ---------//


//--------- Begin of function Firm::read_derived_file ---------//
//
// Read data in derived class.
//
// If the derived Firm don't have any special data,
// just use Firm::read_file(), otherwise make its own derived copy of read_file()
//
int Firm::read_derived_file(File* filePtr)
{
   //--- read data in derived class -----//

   int readSize = firm_array.firm_class_size(firm_id)-sizeof(Firm);

   if( readSize > 0 )
   {
      if( !filePtr->file_read( (char*) this + sizeof(Firm), readSize ) )
         return 0;
   }

   return 1;
}
//----------- End of function Firm::read_derived_file ---------//


//*****//


//-------- Start of function SiteArray::write_file -------------//
//
int SiteArray::write_file(File* filePtr)
{
	filePtr->file_put_short(selected_recno);
	filePtr->file_put_short(untapped_raw_count);
	filePtr->file_put_short(scroll_count);
	filePtr->file_put_short(gold_coin_count);
	filePtr->file_put_short(std_raw_site_count);

	return DynArrayB::write_file( filePtr );
}
//--------- End of function SiteArray::write_file ---------------//


//-------- Start of function SiteArray::read_file -------------//
//
int SiteArray::read_file(File* filePtr)
{
	selected_recno		 = filePtr->file_get_short();
	untapped_raw_count =	filePtr->file_get_short();
	scroll_count		 = filePtr->file_get_short();
	gold_coin_count	 =	filePtr->file_get_short();
	std_raw_site_count =	filePtr->file_get_short();

	return DynArrayB::read_file( filePtr );
}
//--------- End of function SiteArray::read_file ---------------//


//*****//


//-------- Start of function TownArray::write_file -------------//
//
int TownArray::write_file(File* filePtr)
{
   int  i;
   Town *townPtr;

	filePtr->file_put_short( size()  );  // no. of towns in town_array
	filePtr->file_put_short( selected_recno );
	filePtr->file_write( race_wander_pop_array, sizeof(race_wander_pop_array) );

	filePtr->file_put_short( Town::if_town_recno );

	//-----------------------------------------//

	for( i=1; i<=size() ; i++ )
	{
		townPtr = (Town*) get_ptr(i);

      //----- write townId or 0 if the town is deleted -----//

      if( !townPtr )    // the town is deleted
      {
         filePtr->file_put_short(0);
      }
      else
		{
			#ifdef DEBUG
				townPtr->verify_slot_object_id_array();		// for debugging only
			#endif

			filePtr->file_put_short(1);      // the town exists

         if( !filePtr->file_write( townPtr, sizeof(Town) ) )
            return 0;
      }
   }

   //------- write empty room array --------//

   write_empty_room(filePtr);

   return 1;
}
//--------- End of function TownArray::write_file ---------------//


//-------- Start of function TownArray::read_file -------------//
//
int TownArray::read_file(File* filePtr)
{
   Town*   townPtr;
   int     i;

	int townCount = filePtr->file_get_short();  // get no. of towns from file
	selected_recno = filePtr->file_get_short();

#ifdef AMPLUS
	if(!game_file_array.same_version)
	{
		memset(race_wander_pop_array, 0, sizeof(race_wander_pop_array));
		filePtr->file_read( race_wander_pop_array, sizeof(race_wander_pop_array[0])*VERSION_1_MAX_RACE );
	}
	else
		filePtr->file_read( race_wander_pop_array, sizeof(race_wander_pop_array) );
#else
	filePtr->file_read( race_wander_pop_array, sizeof(race_wander_pop_array) );
#endif

	Town::if_town_recno = filePtr->file_get_short();

	//------------------------------------------//

	for( i=1 ; i<=townCount ; i++ )
	{
		if( filePtr->file_get_short()==0 )  // the town has been deleted
		{
			add_blank(1);     // it's a DynArrayB function
		}
		else
		{
			townPtr = town_array.create_town();

			#ifdef AMPLUS
				if(!game_file_array.same_version)
				{
					Version_1_Town *oldTown = (Version_1_Town*) mem_add(sizeof(Version_1_Town));
					if(!filePtr->file_read(oldTown, sizeof(Version_1_Town)))
					{
						mem_del(oldTown);
						return 0;
					}

					oldTown->convert_to_version_2(townPtr);
					mem_del(oldTown);
				}
				else
				{
					if( !filePtr->file_read( townPtr, sizeof(Town) ) )
						return 0;
				}
			#else
				if( !filePtr->file_read( townPtr, sizeof(Town) ) )
					return 0;
			#endif

			#ifdef DEBUG
				townPtr->verify_slot_object_id_array();		// for debugging only
			#endif
		}
	}

	//-------- linkout() those record added by add_blank() ----------//
	//-- So they will be marked deleted in DynArrayB and can be -----//
	//-- undeleted and used when a new record is going to be added --//

   for( i=size() ; i>0 ; i-- )
   {
      DynArrayB::go(i);             // since TownArray has its own go() which will call GroupArray::go()

      if( get_ptr() == NULL )       // add_blank() record
         linkout();
   }

   //------- read empty room array --------//

   read_empty_room(filePtr);

   return 1;
}
//--------- End of function TownArray::read_file ---------------//


//*****//


//-------- Start of function NationArray::write_file -------------//
//
int NationArray::write_file(File* filePtr)
{
	//------ write info in NationArray ------//

   if( !filePtr->file_write( (char*) this + sizeof(DynArrayB), sizeof(NationArray)-sizeof(DynArrayB) ) )
      return 0;

   //---------- write Nations --------------//

   int    i;
   Nation *nationPtr;

   filePtr->file_put_short( size() );  // no. of nations in nation_array

   for( i=1; i<=size() ; i++ )
   {
      nationPtr = (Nation*) get_ptr(i);

      //----- write nationId or 0 if the nation is deleted -----//

      if( !nationPtr )    // the nation is deleted
      {
         filePtr->file_put_short(0);
      }
      else
      {
         filePtr->file_put_short(1);      // there is a nation in this record

         //------ write data in the base class ------//

         if( !nationPtr->write_file(filePtr) )
            return 0;
      }
   }

   //------- write empty room array --------//

   write_empty_room(filePtr);

   return 1;
}
//--------- End of function NationArray::write_file -------------//


//-------- Start of function NationArray::read_file -------------//
//
int NationArray::read_file(File* filePtr)
{
   //------ read info in NationArray ------//
#ifdef AMPLUS
	if(!game_file_array.same_version)
	{
		Version_1_NationArray *oldNationArrayPtr = (Version_1_NationArray*) mem_add(sizeof(Version_1_NationArray));
		//if( !filePtr->file_read( (char*) oldNationArrayPtr + sizeof(DynArrayB), sizeof(Version_1_NationArray)-sizeof(DynArrayB) ) )
		if( !filePtr->file_read( (char*) oldNationArrayPtr, sizeof(Version_1_NationArray) ) )
		{
			mem_del(oldNationArrayPtr);
			return 0;
		}
		oldNationArrayPtr->convert_to_version_2(this);
		mem_del(oldNationArrayPtr);
	}
	else
	{
		if( !filePtr->file_read( (char*) this + sizeof(DynArrayB), sizeof(NationArray)-sizeof(DynArrayB) ) )
			return 0;
	}
#else
   if( !filePtr->file_read( (char*) this + sizeof(DynArrayB), sizeof(NationArray)-sizeof(DynArrayB) ) )
      return 0;
#endif

   //---------- read Nations --------------//

   int     i, nationRecno, nationCount;
   Nation* nationPtr;

   nationCount = filePtr->file_get_short();  // get no. of nations from file

   for( i=1 ; i<=nationCount ; i++ )
   {
      if( filePtr->file_get_short() == 0 )
      {
         add_blank(1);     // it's a DynArrayB function
      }
      else
      {
         //----- create nation object -----------//

         nationRecno = create_nation();
         nationPtr   = nation_array[nationRecno];

         //----- read data in base class --------//

         if( !nationPtr->read_file( filePtr ) )
            return 0;
      }
   }

   //-------- linkout() those record added by add_blank() ----------//
   //-- So they will be marked deleted in DynArrayB and can be -----//
   //-- undeleted and used when a new record is going to be added --//

   for( i=size() ; i>0 ; i-- )
   {
      DynArrayB::go(i);             // since NationArray has its own go() which will call GroupArray::go()

      if( get_ptr() == NULL )       // add_blank() record
         linkout();
   }

	//-------- set NationArray::player_ptr -----------//

   player_ptr = nation_array[player_recno];

	//------- read empty room array --------//

	read_empty_room(filePtr);

	return 1;
}
//--------- End of function NationArray::read_file ---------------//


//--------- Begin of function Nation::write_file ---------//
//
int Nation::write_file(File* filePtr)
{
	if( !filePtr->file_write( this, sizeof(Nation) ) )
		return 0;

	//----------- write AI Action Array ------------//

	action_array.write_file(filePtr);

	//------ write AI info array ---------//

	write_ai_info(filePtr, ai_town_array, ai_town_count, ai_town_size);

	write_ai_info(filePtr, ai_base_array, ai_base_count, ai_base_size);
	write_ai_info(filePtr, ai_mine_array, ai_mine_count, ai_mine_size);
	write_ai_info(filePtr, ai_factory_array, ai_factory_count, ai_factory_size);
	write_ai_info(filePtr, ai_market_array, ai_market_count, ai_market_size);
	write_ai_info(filePtr, ai_inn_array, ai_inn_count, ai_inn_size);
	write_ai_info(filePtr, ai_camp_array, ai_camp_count, ai_camp_size);
	write_ai_info(filePtr, ai_research_array, ai_research_count, ai_research_size);
	write_ai_info(filePtr, ai_war_array, ai_war_count, ai_war_size);
	write_ai_info(filePtr, ai_harbor_array, ai_harbor_count, ai_harbor_size);

	write_ai_info(filePtr, ai_caravan_array, ai_caravan_count, ai_caravan_size);
	write_ai_info(filePtr, ai_ship_array, ai_ship_count, ai_ship_size);
	write_ai_info(filePtr, ai_general_array, ai_general_count, ai_general_size);

	return 1;
}
//----------- End of function Nation::write_file ---------//


//--------- Begin of static function write_ai_info ---------//
//
static void write_ai_info(File* filePtr, short* aiInfoArray, short aiInfoCount, short aiInfoSize)
{
	filePtr->file_put_short( aiInfoCount );
	filePtr->file_put_short( aiInfoSize  );
	filePtr->file_write( aiInfoArray, sizeof(short) * aiInfoCount );
}
//----------- End of static function write_ai_info ---------//


//--------- Begin of function Nation::read_file ---------//
//
int Nation::read_file(File* filePtr)
{
	char* vfPtr = *((char**)this);      // save the virtual function table pointer

	//---- save the action_array first before loading in the whole Nation class ----//

	char* saveActionArray = (char*) mem_add( sizeof(DynArray) );

	memcpy( saveActionArray, &action_array, sizeof(DynArray) );

	//--------------------------------------------------//
#ifdef AMPLUS
	if(!game_file_array.same_version)
	{
		Version_1_Nation *oldNationPtr = (Version_1_Nation*) mem_add(sizeof(Version_1_Nation));
		if(!filePtr->file_read(oldNationPtr, sizeof(Version_1_Nation)))
		{
			mem_del(oldNationPtr);
			return 0;
		}
		oldNationPtr->convert_to_version_2(this);
		mem_del(oldNationPtr);
	}
	else
	{
		if( !filePtr->file_read( this, sizeof(Nation) ) )
			return 0;
	}
#else
	if( !filePtr->file_read( this, sizeof(Nation) ) )
		return 0;
#endif

	*((char**)this) = vfPtr;

	//---------- restore action_array  ------------//
 
	memcpy( &action_array, saveActionArray, sizeof(DynArray) );

	mem_del( saveActionArray );

	//-------------- read AI Action Array --------------//

	action_array.read_file(filePtr);

	//------ write AI info array ---------//

	read_ai_info(filePtr, &ai_town_array, ai_town_count, ai_town_size);

	read_ai_info(filePtr, &ai_base_array, ai_base_count, ai_base_size);
	read_ai_info(filePtr, &ai_mine_array, ai_mine_count, ai_mine_size);
	read_ai_info(filePtr, &ai_factory_array, ai_factory_count, ai_factory_size);
	read_ai_info(filePtr, &ai_market_array, ai_market_count, ai_market_size);
	read_ai_info(filePtr, &ai_inn_array, ai_inn_count, ai_inn_size);
	read_ai_info(filePtr, &ai_camp_array, ai_camp_count, ai_camp_size);
	read_ai_info(filePtr, &ai_research_array, ai_research_count, ai_research_size);
	read_ai_info(filePtr, &ai_war_array, ai_war_count, ai_war_size);
	read_ai_info(filePtr, &ai_harbor_array, ai_harbor_count, ai_harbor_size);

	read_ai_info(filePtr, &ai_caravan_array, ai_caravan_count, ai_caravan_size);
	read_ai_info(filePtr, &ai_ship_array, ai_ship_count, ai_ship_size);
	read_ai_info(filePtr, &ai_general_array, ai_general_count, ai_general_size);

	return 1;
}
//----------- End of function Nation::read_file ---------//


//--------- Begin of static function read_ai_info ---------//
//
static void read_ai_info(File* filePtr, short** aiInfoArrayPtr, short& aiInfoCount, short& aiInfoSize)
{
	aiInfoCount = filePtr->file_get_short();
	aiInfoSize  = filePtr->file_get_short();

	*aiInfoArrayPtr = (short*) mem_add( aiInfoSize * sizeof(short) );

	filePtr->file_read( *aiInfoArrayPtr, sizeof(short) * aiInfoCount );
}
//----------- End of static function read_ai_info ---------//

//*****//

//-------- Start of function TornadoArray::write_file -------------//
//
int TornadoArray::write_file(File* filePtr)
{
	filePtr->file_put_short(restart_recno);  // variable in SpriteArray

	int    i;
   Tornado *tornadoPtr;

   filePtr->file_put_short( size() );  // no. of tornados in tornado_array

   for( i=1; i<=size() ; i++ )
   {
      tornadoPtr = (Tornado*) get_ptr(i);

      //----- write tornadoId or 0 if the tornado is deleted -----//

      if( !tornadoPtr )    // the tornado is deleted
      {
         filePtr->file_put_short(0);
      }
      else
      {
         filePtr->file_put_short(1);      // there is a tornado in this record

         //------ write data in the base class ------//

         if( !tornadoPtr->write_file(filePtr) )
            return 0;
      }
   }

   //------- write empty room array --------//

   write_empty_room(filePtr);

   return 1;
}
//--------- End of function TornadoArray::write_file -------------//


//-------- Start of function TornadoArray::read_file -------------//
//
int TornadoArray::read_file(File* filePtr)
{
	restart_recno    = filePtr->file_get_short();

   int     i, tornadoRecno, tornadoCount;
   Tornado* tornadoPtr;

   tornadoCount = filePtr->file_get_short();  // get no. of tornados from file

   for( i=1 ; i<=tornadoCount ; i++ )
   {
      if( filePtr->file_get_short() == 0 )
      {
         add_blank(1);     // it's a DynArrayB function
      }
      else
      {
         //----- create tornado object -----------//

         tornadoRecno = tornado_array.create_tornado();
         tornadoPtr   = tornado_array[tornadoRecno];

         //----- read data in base class --------//

         if( !tornadoPtr->read_file( filePtr ) )
            return 0;
      }
   }

   //-------- linkout() those record added by add_blank() ----------//
   //-- So they will be marked deleted in DynArrayB and can be -----//
   //-- undeleted and used when a new record is going to be added --//

   for( i=size() ; i>0 ; i-- )
   {
      DynArrayB::go(i);             // since TornadoArray has its own go() which will call GroupArray::go()

      if( get_ptr() == NULL )       // add_blank() record
         linkout();
   }

   //------- read empty room array --------//

   read_empty_room(filePtr);

   return 1;
}
//--------- End of function TornadoArray::read_file ---------------//


//--------- Begin of function Tornado::write_file ---------//
//
int Tornado::write_file(File* filePtr)
{
   if( !filePtr->file_write( this, sizeof(Tornado) ) )
      return 0;

   return 1;
}
//----------- End of function Tornado::write_file ---------//


//--------- Begin of function Tornado::read_file ---------//
//
int Tornado::read_file(File* filePtr)
{
   char* vfPtr = *((char**)this);      // save the virtual function table pointer

   if( !filePtr->file_read( this, sizeof(Tornado) ) )
      return 0;

   *((char**)this) = vfPtr;

   //------------ post-process the data read ----------//

   sprite_info = sprite_res[sprite_id];

	sprite_info->load_bitmap_res();

	return 1;
}
//----------- End of function Tornado::read_file ---------//


//*****//


//-------- Start of function RebelArray::write_file -------------//
//
int RebelArray::write_file(File* filePtr)
{
	return write_ptr_array(filePtr, sizeof(Rebel));
}
//--------- End of function RebelArray::write_file ---------------//


//-------- Start of function RebelArray::read_file -------------//
//
int RebelArray::read_file(File* filePtr)
{
	return read_ptr_array(filePtr, sizeof(Rebel), create_rebel_func);
}
//--------- End of function RebelArray::read_file ---------------//


//-------- Start of static function create_rebel_func ---------//
//
static char* create_rebel_func()
{
	Rebel *rebelPtr = new Rebel;

	rebel_array.linkin(&rebelPtr);

	return (char*) rebelPtr;
}
//--------- End of static function create_rebel_func ----------//


//*****//


//-------- Start of function SpyArray::write_file -------------//
//
int SpyArray::write_file(File* filePtr)
{
	return DynArrayB::write_file( filePtr );
}
//--------- End of function SpyArray::write_file ---------------//


//-------- Start of function SpyArray::read_file -------------//
//
int SpyArray::read_file(File* filePtr)
{
	return DynArrayB::read_file( filePtr );
}
//--------- End of function SpyArray::read_file ---------------//


//*****//


//-------- Start of function SnowGroundArray::write_file -------------//
//
int SnowGroundArray::write_file(File* filePtr)
{
   if( !filePtr->file_write( this, sizeof(SnowGroundArray)) )
      return 0;

   return 1;
}
//--------- End of function SnowGroundArray::write_file ---------------//


//-------- Start of function SnowGroundArray::read_file -------------//
//
int SnowGroundArray::read_file(File* filePtr)
{
   if( !filePtr->file_read( this, sizeof(SnowGroundArray)) )
      return 0;

   return 1;
}
//--------- End of function SnowGroundArray::read_file ---------------//

//*****//

//-------- Start of function RegionArray::write_file -------------//
//
int RegionArray::write_file(File* filePtr)
{
   if( !filePtr->file_write( this, sizeof(RegionArray)) )
      return 0;

	if( !filePtr->file_write( region_info_array, sizeof(RegionInfo)*region_info_count ) )
		return 0;

	//-------- write RegionStat ----------//

	filePtr->file_put_short( region_stat_count );

	if( !filePtr->file_write( region_stat_array, sizeof(RegionStat)*region_stat_count ) )
		return 0;

	//--------- write connection bits ----------//

	int connectBit = (region_info_count -1) * (region_info_count) /2;
	int connectByte = (connectBit +7) /8;

	if( connectByte > 0)
	{
		if( !filePtr->file_write(connect_bits, connectByte) )
			return 0;
	}

	return 1;
}
//--------- End of function RegionArray::write_file ---------------//


//-------- Start of function RegionArray::read_file -------------//
//
int RegionArray::read_file(File* filePtr)
{
   if( !filePtr->file_read( this, sizeof(RegionArray)) )
      return 0;

   if( region_info_count > 0 )
      region_info_array = (RegionInfo *) mem_add(sizeof(RegionInfo)*region_info_count);
   else
      region_info_array = NULL;

   if( !filePtr->file_read( region_info_array, sizeof(RegionInfo)*region_info_count))
      return 0;

	//-------- read RegionStat ----------//

	region_stat_count = filePtr->file_get_short();

	region_stat_array = (RegionStat*) mem_add( region_stat_count * sizeof(RegionStat) );

	if( !filePtr->file_read( region_stat_array, sizeof(RegionStat)*region_stat_count ) )
		return 0;

	//--------- read connection bits ----------//

	int connectBit = (region_info_count -1) * (region_info_count) /2;
	int connectByte = (connectBit +7) /8;

	if( connectByte > 0)
	{
		connect_bits = (unsigned char *)mem_add(connectByte);
		if( !filePtr->file_read(connect_bits, connectByte) )
			return 0;
	}
	else
	{
		connect_bits = NULL;
	}

	return 1;
}
//--------- End of function RegionArray::read_file ---------------//

//*****//

//-------- Start of function NewsArray::write_file -------------//
//
int NewsArray::write_file(File* filePtr)
{
   //----- save news_array parameters -----//

   filePtr->file_write( news_type_option, sizeof(news_type_option) );

   filePtr->file_put_short(news_who_option);
   filePtr->file_put_long (last_clear_recno);

   //---------- save news data -----------//

   return DynArray::write_file(filePtr);
}
//--------- End of function NewsArray::write_file ---------------//


//-------- Start of function NewsArray::read_file -------------//
//
int NewsArray::read_file(File* filePtr)
{
   //----- read news_array parameters -----//

   filePtr->file_read( news_type_option, sizeof(news_type_option) );

   news_who_option   = (char) filePtr->file_get_short();
   last_clear_recno  = filePtr->file_get_long();

   //---------- read news data -----------//

   return DynArray::read_file(filePtr);
}
//--------- End of function NewsArray::read_file ---------------//


